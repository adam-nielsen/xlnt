#pragma once

#include <iostream>
#include <cxxtest/TestSuite.h>

#include <detail/style_serializer.hpp>
#include <detail/stylesheet.hpp>
#include <detail/workbook_impl.hpp>
#include <helpers/path_helper.hpp>

class test_style_reader : public CxxTest::TestSuite
{
public:
    void test_complex_formatting()
    {
        xlnt::workbook wb;
        wb.load(path_helper::get_data_directory("/reader/formatting.xlsx"));
        
        // border_style
        auto ws = wb.get_active_sheet();
        auto e30 = ws.get_cell("E30");
        TS_ASSERT_EQUALS(e30.get_border().get_top()->get_color(), xlnt::color(xlnt::color::type::indexed, 10));
        TS_ASSERT_EQUALS(e30.get_border().get_top()->get_border_style(), xlnt::border_style::thin);
        
        // underline_style
        auto f30 = ws.get_cell("F30");
        TS_ASSERT_EQUALS(e30.get_font().get_underline(), xlnt::font::underline_style::none);
        TS_ASSERT_EQUALS(f30.get_font().get_underline(), xlnt::font::underline_style::single);
        
        // gradient fill
        auto e21 = ws.get_cell("E21");
        TS_ASSERT_EQUALS(e21.get_fill().get_type(), xlnt::fill::type::gradient);
        TS_ASSERT_EQUALS(e21.get_fill().get_gradient_fill().get_type(), xlnt::gradient_fill::type::linear);
        TS_ASSERT_EQUALS(e21.get_fill().get_gradient_fill().get_stops().size(), 2);
        TS_ASSERT_EQUALS(e21.get_fill().get_gradient_fill().get_stops().at(0), xlnt::color(xlnt::color::type::rgb, "FFFF0000"));
        TS_ASSERT_EQUALS(e21.get_fill().get_gradient_fill().get_stops().at(1), xlnt::color(xlnt::color::type::rgb, "FF0000FF"));
    }
};
