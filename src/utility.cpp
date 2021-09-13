#include "utility.h"
#include <QFile>
#include <QTextStream>

Utility::Utility()
{

}

QString Utility::read_unicode_text_file(const QString &file_full_path)
{
    QFile file(file_full_path);
    QString result;
    // Check if we can open the file
    if (!file.open(QFile::ReadOnly))
    {
        return result;
    }
    QTextStream in(&file);
    // Input should be UTF-8
    in.setCodec("UTF-8");
    // This will automatically switch reading from
    // UTF-8 to UTF-16 if a BOM is detected
    in.setAutoDetectUnicode(true);
    result = in.readAll();
    return result.replace("\x0D\x0A", "\x0A").replace("\x0D", "\x0A");
}

QString Utility::decode_xml(const QString& text)
{
    QString newtext(text);
    newtext.replace("&apos;", "'");
    newtext.replace("&quot;", "\"");
    newtext.replace("&lt;", "<");
    newtext.replace("&gt;", ">");
    newtext.replace("&amp;", "&");
    return newtext;
}
