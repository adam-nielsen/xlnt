// Copyright (c) 2014-2016 Thomas Fussell
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, WRISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE
//
// @license: http://www.opensource.org/licenses/mit-license.php
// @author: see AUTHORS file
#include <limits>

#include <detail/constants.hpp>

#include "xlnt_config.hpp"

namespace xlnt {

const row_t constants::min_row()
{
    return 1;
}
    
const row_t constants::max_row()
{
    return std::numeric_limits<row_t>::max();
}

const column_t constants::min_column()
{
    return column_t(1);
}

const column_t constants::max_column()
{
    return column_t(std::numeric_limits<column_t::index_t>::max());
}

// constants
const std::string constants::package_properties() { return "docProps"; }
const std::string constants::package_xl() { return "xl"; }
const std::string constants::package_root_rels() { return "_rels"; }
const std::string constants::package_theme() { return package_xl() + "/" + "theme"; }
const std::string constants::package_worksheets() { return package_xl() + "/" + "worksheets"; }

const std::string constants::part_content_types() { return "[Content_Types].xml"; }
const std::string constants::part_root_relationships() { return package_root_rels() + "/.rels"; }
const std::string constants::part_core() { return package_properties() + "/core.xml"; }
const std::string constants::part_app() { return package_properties() + "/app.xml"; }
const std::string constants::part_workbook() { return package_xl() + "/workbook.xml"; }
const std::string constants::part_styles() { return package_xl() + "/styles.xml"; }
const std::string constants::part_theme() { return package_theme() + "/theme1.xml"; }
const std::string constants::part_shared_strings() { return package_xl() + "/sharedStrings.xml"; }

const std::unordered_map<std::string, std::string> &constants::get_namespaces()
{
    const std::unordered_map<std::string, std::string> *namespaces =
        new std::unordered_map<std::string, std::string>
        {
            { "spreadsheetml", "http://schemas.openxmlformats.org/spreadsheetml/2006/main" },
            { "content-types", "http://schemas.openxmlformats.org/package/2006/content-types" },
            { "relationships", "http://schemas.openxmlformats.org/package/2006/relationships" },
            { "drawingml", "http://schemas.openxmlformats.org/drawingml/2006/main" },
            { "r", "http://schemas.openxmlformats.org/officeDocument/2006/relationships" },
            { "cp", "http://schemas.openxmlformats.org/package/2006/metadata/core-properties" },
            { "dc", "http://purl.org/dc/elements/1.1/" },
            { "dcterms", "http://purl.org/dc/terms/" },
            { "dcmitype", "http://purl.org/dc/dcmitype/" },
            { "xsi", "http://www.w3.org/2001/XMLSchema-instance" },
            { "vt", "http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes" },
            { "xml", "http://www.w3.org/XML/1998/namespace" }
        };

    return *namespaces;
}

const std::string constants::get_namespace(const std::string &id)
{
    return get_namespaces().find(id)->second;
}

}
