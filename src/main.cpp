#include "epubbook.h"
#include <QCoreApplication>
#include <QDirIterator>
#include <QStandardPaths>
#include <QtDebug>

QMap<QString, QString> read_epub_xml(QString xml_path)
{
    QMap<QString, QString> result;
    QFile f(xml_path);
    f.open(QFile::ReadOnly | QFile::Text);
    auto xml_content = f.readAll();
    //    qDebug() << xml_content;
    QDomDocument doc;
    doc.setContent(xml_content);
    auto root = doc.documentElement();
    auto node = root.firstChild();
    node = node.firstChild();
    while (node.isElement()) {
        auto child_node = node.firstChild();
        QString key, value;
        while (child_node.isElement()) {
            //            qDebug() << child_node.nodeName();
            if (child_node.nodeName() == "itemName") {
                key = child_node.toElement().text();
            }
            if (child_node.nodeName() == "itemValue") {
                value = child_node.toElement().text();
            }
            child_node = child_node.nextSibling();
        }
        if (!key.isEmpty()) {
            result[key] = value;
        }
        node = node.nextSibling();
    }
    f.close();

    return result;
}

int main(int argc, char* argv[])
{
//    QString test_epub_name = QString::fromUtf8(u8"E:/BUG追踪/epub研究/1911ET98639/2013出口消费品质量评价报告（广东卷）.epub");
//    QString test_epub_dir = QString::fromUtf8(u8"e:\\epub_test_out");
//    EpubBook test_book;
//    test_book.open(test_epub_name);

//QString target_dir(u8"E:/BUG追踪/1-接收登记-输出文件");
QString target_dir(u8"E:/EPUB下线数据");
QDirIterator dir_iter(target_dir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
QMap<QString, QString> epub_xml_map;
QList<QString> xml_list;
while (dir_iter.hasNext())
{
    dir_iter.next();
    if (dir_iter.fileInfo().suffix().toLower() == QString::fromUtf8(u8"epub") && dir_iter.fileInfo().filePath().contains(u8"原始文件")) {
        epub_xml_map[dir_iter.filePath()] = "";
        //        EpubBook test_book;
        //        auto epub_path = dir_iter.fileInfo().absoluteFilePath();
        //        test_book.open(epub_path);
    }
    if (dir_iter.fileInfo().suffix().toLower() == "xml") {
        xml_list.append(dir_iter.filePath());
        //        auto code = dir_iter.fileName();
        //        code = code.left(code.indexOf('.'));
        //        qDebug() << code;
        //        for (auto epub_path = epub_xml_map.begin(); epub_path != epub_xml_map.end(); ++epub_path) {
        //        }
    }
}
foreach (auto i, xml_list) {
    auto xml_file_info = QFileInfo(i);
    auto code = xml_file_info.fileName();
    code = code.left(code.indexOf('.'));
    for (auto epub_path = epub_xml_map.begin(); epub_path != epub_xml_map.end(); ++epub_path) {
        //        qDebug() << epub_path.key() << ":" << epub_path.value();
        if (epub_path.key().contains(code)) {
            epub_xml_map[epub_path.key()] = i;
        }
    }
}

QFile result_file(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + "epub_result.txt");
result_file.open(QFile::WriteOnly);
QTextStream text_out(&result_file);
text_out.setCodec("utf-8");
double creator = 0, creator_online = 0;
double publisher = 0, publisher_online = 0;
double isbn = 0, isbn_online = 0;
double title = 0, title_online = 0;
double subject = 0, subject_online = 0;
double description = 0, description_online = 0;
double date = 0, date_online = 0;

for (auto epub_path = epub_xml_map.begin(); epub_path != epub_xml_map.end(); ++epub_path) {
    //    qDebug() << epub_path.key() << ":" << epub_path.value();
    auto code = QFileInfo(epub_path.value()).fileName();
    code = code.left(code.indexOf('.'));
    text_out << code << "\n";
    //    text_out << "**********************" + code + "************************\n";
    text_out << QString::fromUtf8(u8"\t程序\t线上\n");
    if (true) {
        //        qDebug() << *epub_path;
        EpubBook test_book;
        auto temp_path = epub_path.key();
        test_book.open(temp_path);
        auto mate_data_map = test_book.get_the_meta_data_map();
        auto xml_map = read_epub_xml(epub_path.value());
        if (!mate_data_map["dc:creator"].isEmpty()) {
            creator += 1;
        }
        if (!xml_map[u8"作者"].isEmpty()) {
            creator_online += 1;
        }
        text_out << QString::fromUtf8(u8"作者\t") << mate_data_map["dc:creator"] << "\t" << xml_map[u8"作者"] << "\n";
        if (!mate_data_map["dc:publisher"].isEmpty()) {
            publisher += 1;
        }
        if (!xml_map[u8"出版者"].isEmpty()) {
            publisher_online += 1;
        }
        text_out << QString::fromUtf8(u8"出版者\t") << mate_data_map["dc:publisher"] << "\t" << xml_map[u8"出版者"] << "\n";
        if (!mate_data_map["isbn"].isEmpty()) {
            isbn += 1;
        }
        if (!xml_map[u8"ISBN"].isEmpty()) {
            isbn_online += 1;
        }
        text_out << QString::fromUtf8(u8"ISBN\t") << mate_data_map["isbn"] << "\t" << xml_map[u8"ISBN"] << "\n";
        if (!mate_data_map["dc:title"].isEmpty()) {
            title += 1;
        }
        if (!xml_map[u8"书名"].isEmpty()) {
            title_online += 1;
        }
        text_out << QString::fromUtf8(u8"书名\t") << mate_data_map["dc:title"] << "\t" << xml_map[u8"书名"] << "\n";
        if (!mate_data_map["dc:subject"].isEmpty()) {
            subject += 1;
        }
        if (!xml_map[u8"副书名"].isEmpty()) {
            subject_online += 1;
        }
        text_out << QString::fromUtf8(u8"副书名\t") << mate_data_map["dc:subject"] << "\t" << xml_map[u8"副书名"] << "\n";
        if (!mate_data_map["dc:description"].isEmpty()) {
            description += 1;
        }
        if (!xml_map[u8"内容提要"].isEmpty()) {
            description_online += 1;
        }
        text_out << QString::fromUtf8(u8"内容提要\t") << mate_data_map["dc:description"] << "\t" << xml_map[u8"内容提要"].replace("\n", " ") << "\n";
        if (!mate_data_map["dc:date"].isEmpty()) {
            date += 1;
        }
        if (!xml_map[u8"出版日期"].isEmpty()) {
            date_online += 1;
        }
        text_out << QString::fromUtf8(u8"出版日期\t") << mate_data_map["dc:date"] << "\t" << xml_map[u8"出版日期"] << "\n";
        text_out << QString::fromUtf8(u8"版权页\t") << test_book.getCopyritht_content() << "\n";
    }
    //    text_out << "**********************************************\n";
    text_out.flush();
}
qDebug() << epub_xml_map.size();
text_out << QString::fromUtf8(u8"作者字段产出率：\t") << QString::number(creator / epub_xml_map.size()) << ":" << QString::number(creator_online / epub_xml_map.size()) << "\n";
text_out << QString::fromUtf8(u8"出版者字段产出率：\t") << QString::number(publisher / epub_xml_map.size()) << ":" << QString::number(publisher_online / epub_xml_map.size()) << "\n";
text_out << QString::fromUtf8(u8"ISBN字段产出率：\t") << QString::number(isbn / epub_xml_map.size()) << ":" << QString::number(isbn_online / epub_xml_map.size()) << "\n";
text_out << QString::fromUtf8(u8"书名字段产出率：\t") << QString::number(title / epub_xml_map.size()) << ":" << QString::number(title_online / epub_xml_map.size()) << "\n";
text_out << QString::fromUtf8(u8"副书名字段产出率：\t") << QString::number(subject / epub_xml_map.size()) << ":" << QString::number(subject_online / epub_xml_map.size()) << "\n";
text_out << QString::fromUtf8(u8"内容提要字段产出率：\t") << QString::number(description / epub_xml_map.size()) << ":" << QString::number(description_online / epub_xml_map.size()) << "\n";
text_out << QString::fromUtf8(u8"出版日期字段产出率：\t") << QString::number(date / epub_xml_map.size()) << ":" << QString::number(date_online / epub_xml_map.size()) << "\n";
result_file.close();

return 0;
}
