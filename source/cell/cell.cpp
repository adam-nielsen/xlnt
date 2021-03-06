// Copyright (c) 2014-2016 Thomas Fussell
// Copyright (c) 2010-2015 openpyxl
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
#include <algorithm>
#include <cmath>
#include <sstream>

#include <xlnt/cell/cell.hpp>
#include <xlnt/cell/cell_reference.hpp>
#include <xlnt/cell/comment.hpp>
#include <xlnt/cell/text.hpp>
#include <xlnt/packaging/document_properties.hpp>
#include <xlnt/packaging/relationship.hpp>
#include <xlnt/styles/color.hpp>
#include <xlnt/styles/format.hpp>
#include <xlnt/styles/style.hpp>
#include <xlnt/utils/date.hpp>
#include <xlnt/utils/datetime.hpp>
#include <xlnt/utils/time.hpp>
#include <xlnt/utils/timedelta.hpp>
#include <xlnt/utils/exceptions.hpp>
#include <xlnt/workbook/workbook.hpp>
#include <xlnt/worksheet/column_properties.hpp>
#include <xlnt/worksheet/row_properties.hpp>
#include <xlnt/worksheet/worksheet.hpp>

#include <detail/cell_impl.hpp>
#include <detail/comment_impl.hpp>


namespace {

std::pair<bool, long double> cast_numeric(const std::string &s)
{
	const char *str = s.c_str();
	char *str_end = nullptr;
	auto result = std::strtold(str, &str_end);
	if (str_end != str + s.size()) return{ false, 0 };
	return{ true, result };
}

std::pair<bool, long double> cast_percentage(const std::string &s)
{
	if (s.back() == '%')
	{
		auto number = cast_numeric(s.substr(0, s.size() - 1));

		if (number.first)
		{
			return{ true, number.second / 100 };
		}
	}

	return{ false, 0 };
}

std::pair<bool, xlnt::time> cast_time(const std::string &s)
{
	xlnt::time result;

	try
	{
		auto last_colon = s.find_last_of(':');
		if (last_colon == std::string::npos) return{ false, result };
		double seconds = std::stod(s.substr(last_colon + 1));
		result.second = static_cast<int>(seconds);
		result.microsecond = static_cast<int>((seconds - static_cast<double>(result.second)) * 1e6);

		auto first_colon = s.find_first_of(':');

		if (first_colon == last_colon)
		{
			auto decimal_pos = s.find('.');
			if (decimal_pos != std::string::npos)
			{
				result.minute = std::stoi(s.substr(0, first_colon));
			}
			else
			{
				result.hour = std::stoi(s.substr(0, first_colon));
				result.minute = result.second;
				result.second = 0;
			}
		}
		else
		{
			result.hour = std::stoi(s.substr(0, first_colon));
			result.minute = std::stoi(s.substr(first_colon + 1, last_colon - first_colon - 1));
		}
	}
	catch (std::invalid_argument)
	{
		return{ false, result };
	}

	return{ true, result };
}

} // namespace

namespace xlnt {

const std::unordered_map<std::string, int> &cell::error_codes()
{
    static const std::unordered_map<std::string, int> *codes =
    new std::unordered_map<std::string, int>({ { "#NULL!", 0 }, { "#DIV/0!", 1 }, { "#VALUE!", 2 },
                                                                { "#REF!", 3 },  { "#NAME?", 4 },  { "#NUM!", 5 },
                                                                { "#N/A!", 6 } });

    return *codes;
};

std::string cell::check_string(const std::string &to_check)
{
    // so we can modify it
    std::string s = to_check;

    if (s.size() == 0)
    {
        return s;
    }
    else if (s.size() > 32767)
    {
        s = s.substr(0, 32767); // max string length in Excel
    }

    for (char c : s)
    {
        if (c >= 0 && (c <= 8 || c == 11 || c == 12 || (c >= 14 && c <= 31)))
        {
            throw xlnt::illegal_character_error(c);
        }
    }

    return s;
}

cell::cell() : d_(nullptr)
{
}

cell::cell(detail::cell_impl *d) : d_(d)
{
}

cell::cell(worksheet worksheet, const cell_reference &reference) : d_(nullptr)
{
    cell self = worksheet.get_cell(reference);
    d_ = self.d_;
}

template <typename T>
cell::cell(worksheet worksheet, const cell_reference &reference, const T &initial_value) : cell(worksheet, reference)
{
    set_value(initial_value);
}

bool cell::garbage_collectible() const
{
    return !(get_data_type() != type::null || is_merged() || has_comment() || has_formula() || has_format());
}

template <>
XLNT_FUNCTION void cell::set_value(bool b)
{
    d_->value_numeric_ = b ? 1 : 0;
    d_->type_ = type::boolean;
}

template <>
XLNT_FUNCTION void cell::set_value(std::int8_t i)
{
    d_->value_numeric_ = static_cast<long double>(i);
    d_->type_ = type::numeric;
}

template <>
XLNT_FUNCTION void cell::set_value(std::int16_t i)
{
    d_->value_numeric_ = static_cast<long double>(i);
    d_->type_ = type::numeric;
}

template <>
XLNT_FUNCTION void cell::set_value(std::int32_t i)
{
    d_->value_numeric_ = static_cast<long double>(i);
    d_->type_ = type::numeric;
}

template <>
XLNT_FUNCTION void cell::set_value(std::int64_t i)
{
    d_->value_numeric_ = static_cast<long double>(i);
    d_->type_ = type::numeric;
}

template <>
XLNT_FUNCTION void cell::set_value(std::uint8_t i)
{
    d_->value_numeric_ = static_cast<long double>(i);
    d_->type_ = type::numeric;
}

template <>
XLNT_FUNCTION void cell::set_value(std::uint16_t i)
{
    d_->value_numeric_ = static_cast<long double>(i);
    d_->type_ = type::numeric;
}

template <>
XLNT_FUNCTION void cell::set_value(std::uint32_t i)
{
    d_->value_numeric_ = static_cast<long double>(i);
    d_->type_ = type::numeric;
}

template <>
XLNT_FUNCTION void cell::set_value(std::uint64_t i)
{
    d_->value_numeric_ = static_cast<long double>(i);
    d_->type_ = type::numeric;
}

#ifdef _MSC_VER
template <>
XLNT_FUNCTION void cell::set_value(unsigned long i)
{
    d_->value_numeric_ = static_cast<long double>(i);
    d_->type_ = type::numeric;
}
#endif

#ifdef __linux
template <>
XLNT_FUNCTION void cell::set_value(long long i)
{
    d_->value_numeric_ = static_cast<long double>(i);
    d_->type_ = type::numeric;
}

template <>
XLNT_FUNCTION void cell::set_value(unsigned long long i)
{
    d_->value_numeric_ = static_cast<long double>(i);
    d_->type_ = type::numeric;
}
#endif

template <>
XLNT_FUNCTION void cell::set_value(float f)
{
    d_->value_numeric_ = static_cast<long double>(f);
    d_->type_ = type::numeric;
}

template <>
XLNT_FUNCTION void cell::set_value(double d)
{
    d_->value_numeric_ = static_cast<long double>(d);
    d_->type_ = type::numeric;
}

template <>
XLNT_FUNCTION void cell::set_value(long double d)
{
    d_->value_numeric_ = static_cast<long double>(d);
    d_->type_ = type::numeric;
}

template <>
XLNT_FUNCTION void cell::set_value(std::string s)
{
	s = check_string(s);

	if (s.size() > 1 && s.front() == '=')
	{
		d_->type_ = type::formula;
		set_formula(s);
	}
	else if (cell::error_codes().find(s) != cell::error_codes().end())
	{
		set_error(s);
	}
	else
	{
		d_->type_ = type::string;
        d_->value_text_.set_plain_string(s);
        
        if (s.size() > 0)
        {
            get_workbook().add_shared_string(d_->value_text_);
        }
	}

	if (get_workbook().get_guess_types())
	{
		guess_type_and_set_value(s);
	}
}

template <>
XLNT_FUNCTION void cell::set_value(text t)
{
    if (t.get_runs().size() == 1 && !t.get_runs().front().has_formatting())
    {
        set_value(t.get_plain_string());
    }
    else
    {
        d_->type_ = type::string;
        d_->value_text_ = t;
        get_workbook().add_shared_string(t);
    }
}

template <>
XLNT_FUNCTION void cell::set_value(char const *c)
{
    set_value(std::string(c));
}

template <>
XLNT_FUNCTION void cell::set_value(cell c)
{
    d_->type_ = c.d_->type_;
    d_->value_numeric_ = c.d_->value_numeric_;
    d_->value_text_ = c.d_->value_text_;
    d_->hyperlink_ = c.d_->hyperlink_;
    d_->has_hyperlink_ = c.d_->has_hyperlink_;
    d_->formula_ = c.d_->formula_;
    d_->format_id_ = c.d_->format_id_;
    if (c.has_comment()) set_comment(c.get_comment());
}

template <>
XLNT_FUNCTION void cell::set_value(date d)
{
    d_->type_ = type::numeric;
    d_->value_numeric_ = d.to_number(get_base_date());
    set_number_format(number_format::date_yyyymmdd2());
}

template <>
XLNT_FUNCTION void cell::set_value(datetime d)
{
    d_->type_ = type::numeric;
    d_->value_numeric_ = d.to_number(get_base_date());
    set_number_format(number_format::date_datetime());
}

template <>
XLNT_FUNCTION void cell::set_value(time t)
{
    d_->type_ = type::numeric;
    d_->value_numeric_ = t.to_number();
    set_number_format(number_format::date_time6());
}

template <>
XLNT_FUNCTION void cell::set_value(timedelta t)
{
    d_->type_ = type::numeric;
    d_->value_numeric_ = t.to_number();
    set_number_format(number_format("[hh]:mm:ss"));
}

row_t cell::get_row() const
{
    return d_->row_;
}

column_t cell::get_column() const
{
    return d_->column_;
}

void cell::set_merged(bool merged)
{
    d_->is_merged_ = merged;
}

bool cell::is_merged() const
{
    return d_->is_merged_;
}

bool cell::is_date() const
{
    return get_data_type() == type::numeric && get_number_format().is_date_format();
}

cell_reference cell::get_reference() const
{
    return { d_->column_, d_->row_ };
}

bool cell::operator==(std::nullptr_t) const
{
    return d_ == nullptr;
}

bool cell::operator==(const cell &comparand) const
{
    return d_ == comparand.d_;
}

cell &cell::operator=(const cell &rhs)
{
    *d_ = *rhs.d_;
    return *this;
}

std::string cell::to_repr() const
{
    return "<Cell " + worksheet(d_->parent_).get_title() + "." + get_reference().to_string() + ">";
}

relationship cell::get_hyperlink() const
{
    if (!d_->has_hyperlink_)
    {
        throw std::runtime_error("no hyperlink set");
    }

    return d_->hyperlink_;
}

bool cell::has_hyperlink() const
{
    return d_->has_hyperlink_;
}

void cell::set_hyperlink(const std::string &hyperlink)
{
    if (hyperlink.length() == 0 || std::find(hyperlink.begin(), hyperlink.end(), ':') == hyperlink.end())
    {
        throw data_type_error();
    }

    d_->has_hyperlink_ = true;
    d_->hyperlink_ = worksheet(d_->parent_).create_relationship(relationship::type::hyperlink, hyperlink);

    if (get_data_type() == type::null)
    {
        set_value(hyperlink);
    }
}

void cell::set_formula(const std::string &formula)
{
    if (formula.length() == 0)
    {
        throw data_type_error();
    }

    if (formula[0] == '=')
    {
        d_->formula_ = formula.substr(1);
    }
    else
    {
        d_->formula_ = formula;
    }
}

bool cell::has_formula() const
{
    return !d_->formula_.empty();
}

std::string cell::get_formula() const
{
    if (d_->formula_.empty())
    {
        throw data_type_error();
    }

    return d_->formula_;
}

void cell::clear_formula()
{
    d_->formula_.clear();
}

void cell::set_comment(const xlnt::comment &c)
{
    if (c.d_ != d_->comment_.get())
    {
        throw xlnt::attribute_error();
    }

    if (!has_comment())
    {
        get_worksheet().increment_comments();
    }

    *get_comment().d_ = *c.d_;
}

void cell::clear_comment()
{
    if (has_comment())
    {
        get_worksheet().decrement_comments();
    }

    d_->comment_ = nullptr;
}

bool cell::has_comment() const
{
    return d_->comment_ != nullptr;
}

void cell::set_error(const std::string &error)
{
    if (error.length() == 0 || error[0] != '#')
    {
        throw data_type_error();
    }

    d_->value_text_.set_plain_string(error);
    d_->type_ = type::error;
}

cell cell::offset(int column, int row)
{
    return get_worksheet().get_cell(cell_reference(d_->column_ + column, d_->row_ + row));
}

worksheet cell::get_worksheet()
{
    return worksheet(d_->parent_);
}

const worksheet cell::get_worksheet() const
{
    return worksheet(d_->parent_);
}

workbook &cell::get_workbook()
{
	return get_worksheet().get_workbook();
}

const workbook &cell::get_workbook() const
{
	return get_worksheet().get_workbook();
}
comment cell::get_comment()
{
    if (d_->comment_ == nullptr)
    {
        d_->comment_.reset(new detail::comment_impl());
        get_worksheet().increment_comments();
    }

    return comment(d_->comment_.get());
}

//TODO: this shares a lot of code with worksheet::get_point_pos, try to reduce repition
std::pair<int, int> cell::get_anchor() const
{
    static const double DefaultColumnWidth = 51.85;
    static const double DefaultRowHeight = 15.0;

    auto points_to_pixels = [](long double value, long double dpi)
    {
        return static_cast<int>(std::ceil(value * dpi / 72));
    };
    
    auto left_columns = d_->column_ - 1;
    int left_anchor = 0;
    auto default_width = points_to_pixels(DefaultColumnWidth, 96.0);

    for (column_t column_index = 1; column_index <= left_columns; column_index++)
    {
        if (get_worksheet().has_column_properties(column_index))
        {
            auto cdw = get_worksheet().get_column_properties(column_index).width;

            if (cdw > 0)
            {
                left_anchor += points_to_pixels(cdw, 96.0);
                continue;
            }
        }

        left_anchor += default_width;
    }

    auto top_rows = d_->row_ - 1;
    int top_anchor = 0;
    auto default_height = points_to_pixels(DefaultRowHeight, 96.0);

    for (row_t row_index = 1; row_index <= top_rows; row_index++)
    {
        if (get_worksheet().has_row_properties(row_index))
        {
            auto rdh = get_worksheet().get_row_properties(row_index).height;

            if (rdh > 0)
            {
                top_anchor += points_to_pixels(rdh, 96.0);
                continue;
            }
        }

        top_anchor += default_height;
    }

    return { left_anchor, top_anchor };
}

cell::type cell::get_data_type() const
{
    return d_->type_;
}

void cell::set_data_type(type t)
{
    d_->type_ = t;
}

const number_format &cell::get_number_format() const
{
    if (d_->has_format_)
    {
        return get_workbook().get_format(d_->format_id_).get_number_format();
    }
    else
    {
        return get_workbook().get_format(0).get_number_format();
    }
}

const font &cell::get_font() const
{
    if (d_->has_format_)
    {
        return get_workbook().get_format(d_->format_id_).get_font();
    }
    else
    {
        return get_workbook().get_format(0).get_font();
    }
}

const fill &cell::get_fill() const
{
    if (d_->has_format_)
    {
        return get_workbook().get_format(d_->format_id_).get_fill();
    }
    else
    {
        return get_workbook().get_format(0).get_fill();
    }
}

const border &cell::get_border() const
{
    if (d_->has_format_)
    {
        return get_workbook().get_format(d_->format_id_).get_border();
    }
    else
    {
        return get_workbook().get_format(0).get_border();
    }
}

const alignment &cell::get_alignment() const
{
    if (d_->has_format_)
    {
        return get_workbook().get_format(d_->format_id_).get_alignment();
    }
    else
    {
        return get_workbook().get_format(0).get_alignment();
    }
}

const protection &cell::get_protection() const
{
    if (d_->has_format_)
    {
        return get_workbook().get_format(d_->format_id_).get_protection();
    }
    else
    {
        return get_workbook().get_format(0).get_protection();
    }
}

void cell::clear_value()
{
    d_->value_numeric_ = 0;
    d_->value_text_.clear();
    d_->formula_.clear();
    d_->type_ = cell::type::null;
}

template <>
XLNT_FUNCTION bool cell::get_value() const
{
    return d_->value_numeric_ != 0;
}

template <>
XLNT_FUNCTION std::int8_t cell::get_value() const
{
    return static_cast<std::int8_t>(d_->value_numeric_);
}

template <>
XLNT_FUNCTION std::int16_t cell::get_value() const
{
    return static_cast<std::int16_t>(d_->value_numeric_);
}

template <>
XLNT_FUNCTION std::int32_t cell::get_value() const
{
    return static_cast<std::int32_t>(d_->value_numeric_);
}

template <>
XLNT_FUNCTION std::int64_t cell::get_value() const
{
    return static_cast<std::int64_t>(d_->value_numeric_);
}

template <>
XLNT_FUNCTION std::uint8_t cell::get_value() const
{
    return static_cast<std::uint8_t>(d_->value_numeric_);
}

template <>
XLNT_FUNCTION std::uint16_t cell::get_value() const
{
    return static_cast<std::uint16_t>(d_->value_numeric_);
}

template <>
XLNT_FUNCTION std::uint32_t cell::get_value() const
{
    return static_cast<std::uint32_t>(d_->value_numeric_);
}

template <>
XLNT_FUNCTION std::uint64_t cell::get_value() const
{
    return static_cast<std::uint64_t>(d_->value_numeric_);
}

#ifdef __linux
template <>
XLNT_FUNCTION long long cell::get_value() const
{
    return static_cast<long long>(d_->value_numeric_);
}

template <>
XLNT_FUNCTION unsigned long long cell::get_value() const
{
    return static_cast<unsigned long long>(d_->value_numeric_);
}
#endif

template <>
XLNT_FUNCTION float cell::get_value() const
{
    return static_cast<float>(d_->value_numeric_);
}

template <>
XLNT_FUNCTION double cell::get_value() const
{
    return static_cast<double>(d_->value_numeric_);
}

template <>
XLNT_FUNCTION long double cell::get_value() const
{
    return d_->value_numeric_;
}

template <>
XLNT_FUNCTION time cell::get_value() const
{
    return time::from_number(d_->value_numeric_);
}

template <>
XLNT_FUNCTION datetime cell::get_value() const
{
    return datetime::from_number(d_->value_numeric_, get_base_date());
}

template <>
XLNT_FUNCTION date cell::get_value() const
{
    return date::from_number(static_cast<int>(d_->value_numeric_), get_base_date());
}

template <>
XLNT_FUNCTION timedelta cell::get_value() const
{
    return timedelta::from_number(d_->value_numeric_);
}

void cell::set_border(const xlnt::border &border_)
{
    d_->has_format_ = true;
    auto format_copy = get_workbook().get_format(d_->format_id_);
    format_copy.set_border(border_);
    d_->format_id_ = get_workbook().add_format(format_copy);
}

void cell::set_fill(const xlnt::fill &fill_)
{
    d_->has_format_ = true;
    auto format_copy = get_workbook().get_format(d_->format_id_);
    format_copy.set_fill(fill_);
    d_->format_id_ = get_workbook().add_format(format_copy);
}

void cell::set_font(const font &font_)
{
    d_->has_format_ = true;
    auto format_copy = get_workbook().get_format(d_->format_id_);
    format_copy.set_font(font_);
    d_->format_id_ = get_workbook().add_format(format_copy);
}

void cell::set_number_format(const number_format &number_format_)
{
    format new_format;
    
    if (d_->has_format_)
    {
        new_format = get_workbook().get_format(d_->format_id_);
    }
    
    auto number_format_with_id = number_format_;
    
    if (!number_format_with_id.has_id())
    {
        number_format_with_id.set_id(get_worksheet().next_custom_number_format_id());
    }
    
    new_format.set_number_format(number_format_with_id);
    
    d_->has_format_ = true;
    d_->format_id_ = get_workbook().add_format(new_format);
}

void cell::set_alignment(const xlnt::alignment &alignment_)
{
    d_->has_format_ = true;
    auto format_copy = get_workbook().get_format(d_->format_id_);
    format_copy.set_alignment(alignment_);
    d_->format_id_ = get_workbook().add_format(format_copy);
}

void cell::set_protection(const xlnt::protection &protection_)
{
    d_->has_format_ = true;
    auto format_copy = get_workbook().get_format(d_->format_id_);
    format_copy.set_protection(protection_);
    d_->format_id_ = get_workbook().add_format(format_copy);
}

template <>
XLNT_FUNCTION std::string cell::get_value() const
{
    return d_->value_text_.get_plain_string();
}

template <>
XLNT_FUNCTION text cell::get_value() const
{
    return d_->value_text_;
}

bool cell::has_value() const
{
    return d_->type_ != cell::type::null;
}

std::string cell::to_string() const
{
    auto nf = get_number_format();

    switch (get_data_type())
    {
    case cell::type::null:
        return "";
    case cell::type::numeric:
        return get_number_format().format(get_value<long double>(), get_base_date());
    case cell::type::string:
    case cell::type::formula:
    case cell::type::error:
        return get_number_format().format(get_value<std::string>());
    case cell::type::boolean:
        return get_value<long double>() == 0 ? "FALSE" : "TRUE";
    default:
        return "";
    }
}

format &cell::get_format()
{
    return get_workbook().get_format(d_->format_id_);
}

bool cell::has_format() const
{
    return d_->has_format_;
}

void cell::set_format(const format &new_format)
{
    d_->format_id_ = get_workbook().add_format(new_format);
    d_->has_format_ = true;
}

calendar cell::get_base_date() const
{
    return get_workbook().get_properties().excel_base_date;
}

std::ostream &operator<<(std::ostream &stream, const xlnt::cell &cell)
{
    return stream << cell.to_string();
}

std::size_t cell::get_format_id() const
{
    return d_->format_id_;
}

void cell::guess_type_and_set_value(const std::string &value)
{
	auto percentage = cast_percentage(value);

	if (percentage.first)
	{
		d_->value_numeric_ = percentage.second;
		d_->type_ = cell::type::numeric;
		set_number_format(xlnt::number_format::percentage());
	}
	else
	{
		auto time = cast_time(value);

		if (time.first)
		{
			d_->type_ = cell::type::numeric;
			set_number_format(number_format::date_time6());
			d_->value_numeric_ = time.second.to_number();
		}
		else
		{
			auto numeric = cast_numeric(value);

			if (numeric.first)
			{
				d_->value_numeric_ = numeric.second;
				d_->type_ = cell::type::numeric;
			}
		}
	}
}

void cell::clear_format()
{
    d_->format_id_ = 0;
    d_->has_format_ = false;
}

void cell::clear_style()
{
    d_->style_id_ = 0;
    d_->has_style_ = false;
}

void cell::set_style(const style &new_style)
{
    d_->has_style_ = true;

    if (get_workbook().has_style(new_style.get_name()))
    {
        d_->style_id_ = get_workbook().get_style_id(new_style.get_name());
    }
    else
    {
        d_->style_id_ = get_workbook().add_style(new_style);
    }
}

void cell::set_style(const std::string &style_name)
{
    d_->has_style_ = true;
    
    if (!get_workbook().has_style(style_name))
    {
        throw std::runtime_error("style " + style_name + " doesn't exist in workbook");
    }
    
    d_->style_id_ = get_workbook().get_style_id(style_name);
}

const style &cell::get_style() const
{
    if (!d_->has_style_)
    {
        throw std::runtime_error("cell has no style");
    }

    return get_workbook().get_style_by_id(d_->style_id_);
}

bool cell::has_style() const
{
    return d_->has_style_;
}

} // namespace xlnt
