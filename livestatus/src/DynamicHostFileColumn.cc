// Copyright (C) 2019 tribe29 GmbH - License: GNU General Public License v2
// This file is part of Checkmk (https://checkmk.com). It is subject to the
// terms and conditions defined in the file COPYING, which is part of this
// source code package.

#include "DynamicHostFileColumn.h"

#include <filesystem>
#include <stdexcept>
#include <utility>

#include "FileSystemHelper.h"
#include "HostFileColumn.h"  // IWYU pragma: keep

template <class T>
DynamicHostFileColumn<T>::DynamicHostFileColumn(
    const std::string &name, const std::string &description,
    Column::Offsets offsets, std::function<std::filesystem::path()> basepath,
    std::function<std::filesystem::path(const T &, const std::string &)>
        filepath)
    : DynamicColumn(name, description, std::move(offsets))
    , _basepath{std::move(basepath)}
    , _filepath{std::move(filepath)} {}

template <class T>
[[nodiscard]] std::filesystem::path DynamicHostFileColumn<T>::basepath() const {
    // This delays the call to mc to after it is constructed.
    return _basepath();
}

template <class T>
std::unique_ptr<Column> DynamicHostFileColumn<T>::createColumn(
    const std::string &name, const std::string &arguments) {
    // Arguments contains a path relative to basepath and possibly escaped.
    if (arguments.empty()) {
        throw std::runtime_error("invalid arguments for column '" + _name +
                                 "': missing file name");
    }
    const std::filesystem::path f{mk::unescape_filename(arguments)};
    if (!mk::path_contains(basepath(), basepath() / f)) {
        // Prevent malicious attempts to read files as root with
        // "/etc/shadow" (abs paths are not stacked) or
        // "../../../../etc/shadow".
        throw std::runtime_error("invalid arguments for column '" + _name +
                                 "': '" + f.string() + "' not in '" +
                                 basepath().string() + "'");
    }
    return std::make_unique<HostFileColumn<T>>(
        name, _description, _offsets, _basepath,
        [this, f](const T &r) { return _filepath(r, f); });
}
