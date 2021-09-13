#ifndef UTILITY_H
#define UTILITY_H

#include <QString>

class Utility
{
public:
    Utility();
    //以utf-8的编码方式读入文本文件
    static QString read_unicode_text_file(const QString &file_full_path);
    //处理xml中的特殊字符&->&amp; '->&apos; "->&quot < ->&lt; > -> &gt;
    static QString decode_xml(const QString& text);
};

#endif // UTILITY_H
