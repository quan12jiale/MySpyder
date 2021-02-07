#pragma once

#include "widgets/mixins.h"

void test_get_unicode_regexp()
{
    BaseEditMixin widget;

    QString code = "print(\"И\")\nfoo(\"И\")";
    widget.setPlainText(code);
    QTextCursor cursor = widget.textCursor();
    cursor.setPosition(widget.get_position("sof"));
    Q_ASSERT(widget.find_text("t.*И",true,true,false,false,true));
    Q_ASSERT(widget.get_number_matches("t.*И",code,false,true) == 1);
}

void test_get_number_matches()
{
    BaseEditMixin widget;

    QString code = "class C():\n"
                   "    def __init__(self):\n"
                   "        pass\n"
                   "    def f(self, a, b):\n"
                   "        pass\n";

    //# Empty pattern.
    Q_ASSERT(widget.get_number_matches("") == 0);

    //# No case, no regexp.
    Q_ASSERT(widget.get_number_matches("self",code) == 2);
    Q_ASSERT(widget.get_number_matches("c",code) == 2);

    //# Case, no regexp.
    Q_ASSERT(widget.get_number_matches("self",code,true) == 2);
    Q_ASSERT(widget.get_number_matches("c",code,true) == 1);

    //# No case, regexp.
    Q_ASSERT(widget.get_number_matches("e[a-z]?f",code,false,true) == 4);
    Q_ASSERT(widget.get_number_matches("e[A-Z]?f",code,false,true) == 4);

    //# Case, regexp.
    Q_ASSERT(widget.get_number_matches("e[a-z]?f",code,true,true) == 4);
    Q_ASSERT(widget.get_number_matches("e[A-Z]?f",code,true,true) == 2);

    //# Issue 5680.
    Q_ASSERT(widget.get_number_matches("(",code) == 3);
    Q_ASSERT(widget.get_number_matches("(",code,true) == 3);
    Q_ASSERT(widget.get_number_matches("(",code,false,true) == -1);
    Q_ASSERT(widget.get_number_matches("(",code,true,true) == -1);//-1表示正则表达式patterm不合法
}

void test_get_match_number()
{
    BaseEditMixin widget;

    QString code = "class C():\n"
                   "    def __init__(self):\n"
                   "        pass\n"
                   "    def f(self, a, b):\n"
                   "        pass\n";
    widget.setPlainText(code);
    QTextCursor cursor = widget.textCursor();
    cursor.setPosition(widget.get_position("sof"));

    Q_ASSERT(widget.get_match_number("") == 0);

    widget.find_text("(");
    Q_ASSERT(widget.get_match_number("(") == 1);

    //# First occurrence.
    widget.find_text("self");
    Q_ASSERT(widget.get_match_number("self") == 1);
    Q_ASSERT(widget.get_match_number("self",true) == 1);

    //# Second occurrence.
    widget.find_text("pass");
    widget.find_text("self");
    Q_ASSERT(widget.get_match_number("self") == 2);
    Q_ASSERT(widget.get_match_number("self",true) == 2);
}
