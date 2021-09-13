#include "epubbook.h"
#include <QUrl>
#include <QXmlStreamReader>

#include <iostream>

#include "gumbo.h"
#include "mediatypes.h"
#include "utility.h"

EpubBook::EPUB_STATE EpubBook::open(QString& epub_path)
{
    if(epub_path.isEmpty())
    {
        EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_PATH_NULL);
    }
    if(!epub_path.toLower().endsWith(EPUB_SUFFIX))
    {
        EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_WRONG_SUFFIX);
    }
    QFileInfo epub_info(epub_path);
    if(!epub_info.isFile())
    {
        EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_FILE_MISS);
    }
    m_epub_name = epub_info.baseName();
    m_epub_book_path = epub_path;
    EPUB_STATE ret = extract();
    if(EPUB_STATE::OK != ret)
    {
        EPUB_BOOK_RETRUN(ret);
    }
//    foreach(auto i,m_epub_container_file_list)
//    {
//        qDebug() << "EPUB FILE:" << i;
//    }
    ret = process_container();
    if(EPUB_STATE::OK != ret)
    {
        EPUB_BOOK_RETRUN(ret);
    }
//    if(m_epub_file_structure_map.contains(EPUB_STRUCTURE_OPF))
//    {
//        auto li = m_epub_file_structure_map.find(EPUB_STRUCTURE_OPF).value();
//        for(auto it = li.begin();it != li.end();++it)
//        {
//            qDebug() << "EPUB OPF FILE:" << *it;
//        }
//    }
    qDebug() << m_epub_opf_file_path;
    ret = process_opf();
    ret = locate_copyright_page();

    EPUB_BOOK_RETRUN(EPUB_STATE::OK);
}

QMap<QString, QString> EpubBook::get_the_meta_data_map() const
{
    return m_meta_data_map;
}

EpubBook::EPUB_STATE EpubBook::extract()
{
    int res = 0;
    if (!cp437)
    {
        cp437 = new QCodePage437Codec();
    }
#ifdef Q_OS_WIN32
    zlib_filefunc64_def ffunc;
    fill_win32_filefunc64W(&ffunc);
    auto zip_file_path = QDir::toNativeSeparators(m_epub_book_path);
    unzFile zfile = unzOpen2_64(std::wstring((const wchar_t *)zip_file_path.utf16()).c_str(), &ffunc);
#else
    unzFile zfile = unzOpen64(QDir::toNativeSeparators(m_FullFilePath).toUtf8().constData());
#endif

    if (zfile == NULL)
    {
        EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_UNZIP);
    }

    res = unzGoToFirstFile(zfile);

    if (res == UNZ_OK)
    {
        do
        {
            // Get the name of the file in the archive.
            char file_name[MAX_PATH] = {0};
            unz_file_info64 file_info;
            unzGetCurrentFileInfo64(zfile, &file_info, file_name, MAX_PATH, NULL, 0, NULL, 0);
            QString qfile_name;
            QString cp437_file_name;
            qfile_name = QString::fromUtf8(file_name);
            if (!(file_info.flag & (1<<11)))
            {
                // General purpose bit 11 says the filename is utf-8 encoded. If not set then
                // IBM 437 encoding might be used.
                cp437_file_name = cp437->toUnicode(file_name);
            }

            // If there is no file name then we can't do anything with it.
            if (!qfile_name.isEmpty())
            {

                // for security reasons against maliciously crafted zip archives
                // we need the file path to always be inside the target folder
                // and not outside, so we will remove all illegal backslashes
                // and all relative upward paths segments "/../" from the zip's local
                // file name/path before prepending the target folder to create
                // the final path

                QString original_path = qfile_name;
                bool evil_or_corrupt_epub = false;

                if (qfile_name.contains(BACK_SLASH)) evil_or_corrupt_epub = true;
                {
                    qfile_name = SLASH + qfile_name.replace(BACK_SLASH,"");
                }

                if (qfile_name.contains("/../")) evil_or_corrupt_epub = true;
                {
                    qfile_name = qfile_name.replace("/../",SLASH);
                }

                while(qfile_name.startsWith(SLASH))
                {
                    qfile_name = qfile_name.remove(0,1);
                }

                if (cp437_file_name.contains(BACK_SLASH)) evil_or_corrupt_epub = true;
                {
                    cp437_file_name = SLASH + cp437_file_name.replace(BACK_SLASH,"");
                }

                if (cp437_file_name.contains("/../")) evil_or_corrupt_epub = true;
                {
                    cp437_file_name = cp437_file_name.replace("/../",SLASH);
                }

                while(cp437_file_name.startsWith(SLASH))
                {
                    cp437_file_name = cp437_file_name.remove(0,1);
                }

                if (evil_or_corrupt_epub)
                {
                    unzCloseCurrentFile(zfile);
                    unzClose(zfile);
                    EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_PATH_CORRUPT);
                }

                QStringList m_ZipFilePaths;
                // We use the dir object to create the path in the temporary directory.
                // Unfortunately, we need a dir ojbect to do this as it's not a static function.
                QString epub_target_dir = make_target_dir();
                QDir dir(epub_target_dir);
                // Full file path in the temporary directory.
                QString file_path = epub_target_dir + SLASH + qfile_name;
                QFileInfo qfile_info(file_path);

                // Is this entry a directory?
                if (file_info.uncompressed_size == 0 && qfile_name.endsWith('/'))
                {
                    dir.mkpath(qfile_name);
                    continue;
                }
                else
                {
                    if (!qfile_info.path().isEmpty()) dir.mkpath(qfile_info.path());
                    // add it to the list of files found inside the zip
                    if (cp437_file_name.isEmpty())
                    {
                        m_ZipFilePaths << qfile_name;
                    }
                    else
                    {
                        m_ZipFilePaths << cp437_file_name;
                    }
                }

                // Open the file entry in the archive for reading.
                if (unzOpenCurrentFile(zfile) != UNZ_OK)
                {
                    unzClose(zfile);
                    EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_UNZIP);
                }

                // Open the file on disk to write the entry in the archive to.
                QFile entry(file_path);

                if (!entry.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                    unzCloseCurrentFile(zfile);
                    unzClose(zfile);
                    EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_UNZIP);
                }

                // Buffered reading and writing.
                char buff[BUFF_SIZE] = {0};
                int read = 0;

                while ((read = unzReadCurrentFile(zfile, buff, BUFF_SIZE)) > 0)
                {
                    entry.write(buff, read);
                }

                entry.close();

                // Read errors are marked by a negative read amount.
                if (read < 0)
                {
                    unzCloseCurrentFile(zfile);
                    unzClose(zfile);
                    EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_UNZIP);
                }

                // The file was read but the CRC did not match.
                // We don't check the read file size vs the uncompressed file size
                // because if they're different there should be a CRC error.
                if (unzCloseCurrentFile(zfile) == UNZ_CRCERROR)
                {
                    unzClose(zfile);
                    EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_UNZIP);
                }
                m_epub_container_file_list.append(file_path);
                if (!cp437_file_name.isEmpty() && cp437_file_name != qfile_name)
                {
                    QString cp437_file_path = epub_target_dir + SLASH + cp437_file_name;
                    QFile::copy(file_path, cp437_file_path);
                    m_epub_container_file_list.append(cp437_file_name);
                }
            }
        }
        while ((res = unzGoToNextFile(zfile)) == UNZ_OK);
    }

    if (res != UNZ_END_OF_LIST_OF_FILE)
    {
        unzClose(zfile);
        EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_UNZIP);
    }

    unzClose(zfile);
    EPUB_BOOK_RETRUN(EPUB_STATE::OK);
}

EpubBook::EPUB_STATE EpubBook::process_opf()
{
    QString opf_text = Utility::read_unicode_text_file(m_epub_opf_file_path);
    //    qDebug() << opf_text;
#if 0
    QXmlStreamReader opf_reader(opf_text);
    if (!opf_reader.hasError()) {
        while (!opf_reader.atEnd()) {
            opf_reader.readNext();
            //            qDebug() << opf_reader.name();
            if (opf_reader.isStartElement() && opf_reader.name() == "package") {
                m_epub_version = opf_reader.attributes().value("", "version").toString();
                if (m_epub_version == "1.0") {
                    m_epub_version = "2.0";
                }
                qDebug() << "version:" << m_epub_version;
                m_unique_id = opf_reader.attributes().value("", "unique-identifier").toString();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "metadata") {
            } else if (opf_reader.isStartElement() && opf_reader.name() == "meta") {
                QString name = opf_reader.attributes().value("", "name").toString();
                QString content = opf_reader.attributes().value("", "content").toString();
                qDebug() << "meta name:" << name << "meta content:" << content;
            } else if (opf_reader.isStartElement() && opf_reader.name() == "title") {
                QString dc_title = opf_reader.name().toString();
                qDebug() << "dc:title" << dc_title << "value:" << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "creator") {
                QString dc_creator = opf_reader.name().toString();
                qDebug() << "dc:creator" << dc_creator << "value: " << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "subject") {
                QString dc_subject = opf_reader.name().toString();
                qDebug() << "dc:subject" << dc_subject << "value:" << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "description") {
                QString dc_description = opf_reader.name().toString();
                qDebug() << "dc:description" << dc_description << "value:" << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "contributor") {
                QString dc_contributor = opf_reader.name().toString();
                qDebug() << "dc:contributor" << dc_contributor << "value:" << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "date") {
                QString dc_date = opf_reader.name().toString();
                qDebug() << "dc:date" << dc_date << "value:" << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "type") {
                QString dc_type = opf_reader.name().toString();
                qDebug() << "dc:type" << dc_type << "value:" << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "format") {
                QString dc_format = opf_reader.name().toString();
                qDebug() << "dc:format" << dc_format << "value:" << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "identifier") {
                QString dc_identifier = opf_reader.name().toString();
                QString scheme = opf_reader.attributes().value("", "scheme").toString();
                QString id = opf_reader.attributes().value("", "id").toString();
                QString value = opf_reader.readElementText();
                if (m_unique_id == id) {
                    m_unique_value = value;
                }
                if (m_unique_value.isEmpty() && (value.contains("urn:uuid:") || scheme.toLower() == "uuid")) {
                    m_unique_value = value;
                }
                qDebug() << "dc:identifier " << dc_identifier << "id: " << id << "scheme: " << scheme << "value: " << value;
            } else if (opf_reader.isStartElement() && opf_reader.name() == "source") {
                QString dc_source = opf_reader.name().toString();
                qDebug() << "dc:source" << dc_source << "value:" << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "language") {
                QString dc_language = opf_reader.name().toString();
                qDebug() << "dc:language" << dc_language << "value:" << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "relation") {
                QString dc_relation = opf_reader.name().toString();
                qDebug() << "dc:relation" << dc_relation << "value:" << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "rights") {
                QString dc_rights = opf_reader.name().toString();
                qDebug() << "dc:rights" << dc_rights << "value:" << opf_reader.readElementText();
            } else if (opf_reader.isStartElement() && opf_reader.name() == "publisher") {
                QString dc_publisher = opf_reader.name().toString();
                qDebug() << "dc:publish" << dc_publisher << "value:" << opf_reader.readElementText();
            }
        }
    }
#else
    QDomDocument opf_dom;
    QString dom_error;
    int dom_error_line;
    int dom_error_column;
    if (!opf_dom.setContent(opf_text, false, &dom_error, &dom_error_line, &dom_error_column)) {
        qCritical() << "error:" << dom_error << "line:" << dom_error_line << "col:" << dom_error_column;
        EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_EPUB_OPF);
    }
    auto root = opf_dom.documentElement();
    qDebug() << root.nodeName();
    if (root.nodeName() == "package") {
        m_epub_version = root.attribute("version");
        if (m_epub_version == "1.0") {
            m_epub_version = "2.0";
        }
        qDebug() << "version:" << m_epub_version;
        m_unique_value = root.attribute("unique-identifier");
    }
    QDomNode node = root.firstChild();
    while (!node.isNull()) {
        if (node.isElement()) {
            if (node.nodeName() == "metadata") {
                read_metadata_element(node);
            } else if (node.nodeName() == "manifest") {
                read_manifest_element(node);
            } else if (node.nodeName() == "spine") {
                read_spine_element(node);
            }
        }
        node = node.nextSibling();
    }

#endif
    EPUB_BOOK_RETRUN(EPUB_STATE::OK);
}

EpubBook::EPUB_STATE EpubBook::process_container()
{
    auto container_it = std::find_if(m_epub_container_file_list.begin(),m_epub_container_file_list.end(),[&](QString &x){return x.endsWith(EPUB_CONTAINER_PATH);});
    if(container_it == m_epub_container_file_list.end())
    {
        EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_EPUB_CONTAINER);
    }
    QXmlStreamReader epub_container;
    QString container_text = Utility::read_unicode_text_file(*container_it);
    if(container_text.isEmpty())
    {
        EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_EPUB_CONTAINER);
    }
    //    qDebug() << "CONTAINER CONTENT:" << container_text;
    epub_container.addData(container_text);
    int opf_num = 0;
    while(!epub_container.atEnd())
    {
        epub_container.readNext();
        if(epub_container.isStartElement() && epub_container.name() == "rootfile")
        {
            if(epub_container.attributes().hasAttribute("media-type") &&
                epub_container.attributes().value("","media-type") == OEBPS_MIMETYPE)
            {
                QString temp_opf_path = epub_container.attributes().value("","full-path").toString();
                container_it = std::find_if(m_epub_container_file_list.begin(),m_epub_container_file_list.end(),[&](QString &x){return x.contains(temp_opf_path);});
                if(container_it == m_epub_container_file_list.end())
                {
                    EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_EPUB_OPF_MISS);
                }
                opf_num++;
                if(!m_epub_file_structure_map.contains(EPUB_STRUCTURE_OPF))
                {
                    QList<QString> temp_opf_list;
                    temp_opf_list.append(*container_it);
                    m_epub_file_structure_map.insert(EPUB_STRUCTURE_OPF,temp_opf_list);
                }
                else
                {
                    m_epub_file_structure_map.find(EPUB_STRUCTURE_OPF).value().append(*container_it);
                }
            }
        }
    }
    if(epub_container.hasError())
    {
        EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_EPUB_CONTAINER_CONTENT);
    }
    if(opf_num != 1)
    {
        EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_EPUB_OPF_NUM);
    }
    else
    {
        m_epub_opf_file_path = m_epub_file_structure_map.find(EPUB_STRUCTURE_OPF).value().first();
        if(m_epub_opf_file_path.isEmpty() || !QFile::exists(m_epub_opf_file_path))
        {
            EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_EPUB_OPF);
        }
    }

    EPUB_BOOK_RETRUN(EPUB_STATE::OK);
}

EpubBook::EPUB_STATE EpubBook::process_ncx()
{
    EPUB_BOOK_RETRUN(EPUB_STATE::OK);
}

QString get_all_text_from_html(GumboNode* node)
{
    if (node->type == GUMBO_NODE_TEXT) {
        return QString(node->v.text.text);
    } else if (node->type == GUMBO_NODE_ELEMENT && QString(node->v.element.tag) != "script" && QString(node->v.element.tag) != "style") {
        GumboVector* children = &node->v.element.children;
        QString temp_str;
        for (unsigned int i = 0; i < children->length; ++i) {
            QString child_text = get_all_text_from_html((GumboNode*)children->data[i]);
            if (i != 0 && !child_text.isEmpty()) {
                temp_str.append("\n");
            }
            temp_str.append(child_text);
        }
        return temp_str;
    } else {
        return QString("");
    }
}
int get_body_children_num_from_html(GumboNode* node)
{
    if (node->type == GUMBO_NODE_ELEMENT && QString(gumbo_normalized_tagname(node->v.element.tag)) == "body") {
        return node->v.element.children.length;
    } else {
        return -1;
    }
}

EpubBook::EPUB_STATE EpubBook::locate_copyright_page()
{
    if (m_spine_list.isEmpty()) {
        EPUB_BOOK_RETRUN(EPUB_STATE::ERROR_EPUB_OPF);
    }
    int threshold = 8;

    foreach (auto file_name, m_spine_list) {
        auto full_file_path = std::find_if(m_epub_container_file_list.begin(), m_epub_container_file_list.end(), [&](QString& x) { return x.contains(file_name); });
        if (full_file_path != m_epub_container_file_list.end()) {
            int probability = 0;
            auto file_content = Utility::read_unicode_text_file(*full_file_path);
            if (!file_content.isEmpty()) {
#if 1
                auto file_content_std_string = file_content.toStdString();
                //去除xml头
                if (file_content_std_string.compare(0, 5, "<?xml") == 0) {
                    auto end = file_content_std_string.find_first_of('>', 5);
                    end = file_content_std_string.find_first_not_of("\n\r\t\v\f ", end - 1);
                    file_content_std_string.erase(0, end);
                }
                GumboOptions option;
                option.tab_stop = 4;
                option.use_xhtml_rules = true;
                option.stop_on_first_error = false;
                option.max_tree_depth = 400;
                option.max_errors = 50;
                auto file_content_processed = gumbo_parse_with_options(&option, file_content_std_string.data(), file_content_std_string.length());
                auto root_children = &file_content_processed->root->v.element.children;
                QString page_text;
                for (unsigned int i = 0; i < root_children->length; ++i) {
                    //第一层
                    GumboNode* child = (GumboNode*)root_children->data[i];
                    if (child->type == GUMBO_NODE_ELEMENT && QString(gumbo_normalized_tagname(child->v.element.tag)) == "body") {
                        QString text = get_all_text_from_html(child);
                        //                        if (get_body_children_num_from_html(child) > 0 && get_body_children_num_from_html(child) < 6) {
                        //                            probability += 1;
                        //                        }
                        page_text.append(text);
                    }
                    //                    qDebug() << text;
                }
                gumbo_destroy_output(file_content_processed);
                if (page_text.contains(u8"版权")) {
                    probability += 1;
                }
                if (page_text.contains(u8"发行")) {
                    probability += 1;
                }
                if (m_spine_list.indexOf(file_name) > 0 && m_spine_list.indexOf(file_name) < 10) {
                    probability += 1;
                }
                if (page_text.toLower().contains(u8"isbn")) {
                    probability += 1;
                }
                if (page_text.toLower().contains(u8"copyright")) {
                    probability += 1;
                }
                if (page_text.contains(u8"书名")) {
                    probability += 1;
                }
                if (page_text.contains(u8"作者")) {
                    probability += 1;
                }
                if (page_text.contains(u8"时间")) {
                    probability += 1;
                }
                if (page_text.contains(u8"出版")) {
                    probability += 1;
                }
                if (page_text.contains(u8"侵权")) {
                    probability += 1;
                }

                if (probability >= threshold) {
                    qDebug() << file_name << u8" 这是版权页！";
                    qDebug() << page_text.trimmed();
                    m_copyritht_content = page_text.trimmed().replace("\n", " ");
                }
#endif
            }
        }
    }

    EPUB_BOOK_RETRUN(EPUB_STATE::OK);
}

QString EpubBook::make_target_dir()
{
    return m_epub_temp_folder->path() + SLASH + m_epub_name;
    //    return m_epub_temp_dir +"\\" + m_epub_name;
}

void EpubBook::read_metadata_element(QDomNode node)
{
    node = node.firstChild();
    while (!node.isNull()) {
        if (node.isElement()) {
            //            qDebug() << node.nodeName();
            QDomElement element = node.toElement();
            if (node.nodeName() == "meta") {
                QString name, content;
                if (element.hasAttribute("name")) {
                    name = element.attribute("name");
                }
                if (element.hasAttribute("content")) {
                    content = element.attribute("content");
                }
                qDebug() << "meta name:" << name << "meta content:" << content;
            } else if (node.nodeName() == "dc:title") {
                QString dc_title = element.text();
                m_meta_data_map["dc:title"] = dc_title;
                //                qDebug() << "dc:title " << dc_title;

            } else if (node.nodeName() == "dc:creator") {
                auto dc_creator = element.text();
                m_meta_data_map["dc:creator"] = dc_creator;
                //                qDebug() << "dc:creator" << dc_creator;
            } else if (node.nodeName() == "dc:subject") {
                auto dc_subject = element.text();
                m_meta_data_map["dc:subject"] = dc_subject;
                //                qDebug() << "dc:subject" << dc_subject;
            } else if (node.nodeName() == "dc:description") {
                auto dc_description = element.text();
                m_meta_data_map["dc:description"] = dc_description;
                //                qDebug() << "dc:description" << dc_description;
            } else if (node.nodeName() == "dc:contributor") {
                auto dc_contributor = element.text();
                m_meta_data_map["dc:contributor"] = dc_contributor;
                //                qDebug() << "dc:contributor" << dc_contributor;
            } else if (node.nodeName() == "dc:date") {
                auto dc_date = element.text();
                m_meta_data_map["dc:date"] = dc_date;
                //                qDebug() << "dc:date" << dc_date;
            } else if (node.nodeName() == "dc:type") {
                auto dc_type = element.text();
                m_meta_data_map["dc:type"] = dc_type;
                //                qDebug() << "dc:type" << dc_type;
            } else if (node.nodeName() == "dc:format") {
                auto dc_format = element.text();
                m_meta_data_map["dc:format"] = dc_format;
                //                qDebug() << "dc:format" << dc_format;
            } else if (node.nodeName() == "dc:identifier") {
                auto dc_identifier = element.text();
                QString id, scheme;

                if (element.hasAttribute("id")) {
                    id = element.attribute("id");
                }
                if (element.hasAttribute("opf:scheme")) {
                    scheme = element.attribute("opf:scheme");
                }
                if (m_unique_id == id) {
                    m_unique_value = element.text();
                }
                if (m_unique_value.isEmpty() && (element.text().contains("urn:uuid:") || scheme.toLower() == "uuid")) {
                    m_unique_value = element.text();
                }
                if (id.toLower().contains("isbn") || scheme.toLower().contains("isbn")) {
                    m_meta_data_map["isbn"] = element.text();
                }
                //                qDebug() << "dc:identifier " << dc_identifier << "id: " << id << "scheme: " << scheme;
            } else if (node.nodeName() == "dc:source") {
                auto dc_source = element.text();
                m_meta_data_map["dc:source"] = dc_source;
                //                qDebug() << "dc:source" << dc_source;
            } else if (node.nodeName() == "dc:language") {
                auto dc_language = element.text();
                m_meta_data_map["dc:language"] = dc_language;
                //                qDebug() << "dc:language" << dc_language;
            } else if (node.nodeName() == "dc:relation") {
                auto dc_relation = element.text();
                m_meta_data_map["dc:relation"] = dc_relation;
                //                qDebug() << "dc:relation" << dc_relation;
            } else if (node.nodeName() == "dc:rights") {
                auto dc_rights = element.text();
                m_meta_data_map["dc:rights"] = dc_rights;
                //                qDebug() << "dc:rights" << dc_rights;
            } else if (node.nodeName() == "dc:publisher") {
                auto dc_publisher = element.text();
                m_meta_data_map["dc:publisher"] = dc_publisher;
                //                qDebug() << "dc:publish" << dc_publisher;
            }
        }
        node = node.nextSibling();
    }
}

void EpubBook::read_manifest_element(QDomNode node)
{
    node = node.firstChild();
    while (!node.isNull()) {
        if (node.isElement()) {
            if (node.nodeName() == "item") {
                QString item_href = node.toElement().attribute("href");
                QString item_id = node.toElement().attribute("id");
                QString item_type = node.toElement().attribute("media-type");
                QString item_properties = node.toElement().attribute("properties");
                if (item_href.indexOf(':') == -1) {
                    item_href = Utility::decode_xml(item_href);
                    item_href = QUrl::fromPercentEncoding(item_href.toUtf8());
                }
                QString extension = QFileInfo(item_href).suffix().toLower();
                QString group = MediaTypes::instance()->GetGroupFromMediaType(item_type);
                QString ext_mtype = MediaTypes::instance()->GetMediaTypeFromExtension(extension);
                if (!item_href.isEmpty()) {
                    auto item_path = std::find_if(m_epub_container_file_list.begin(), m_epub_container_file_list.end(), [&](QString& x) { return x.contains(item_href); });
                    if (item_path != m_epub_container_file_list.end()) {
                        if (m_id_path_map.contains(item_id)) {
                            qCritical() << "the same id apperaed!";
                            exit(-1);
                        }
                        //                        qDebug() << "item href->>>>> " << item_href << "group:" << group << "ext_type:" << ext_mtype << "true path:" << *item_path;
                        if (item_type != "application/x-dtbncx+xml" && extension != "ncx") {
                            m_id_path_map[item_id] = *item_path;
                            m_id_mimetype_map[item_id] = item_type;
                            if (item_properties.contains("nav")) {
                                m_catalogue_map["nav"] = *item_path;
                            }
                        } else {
                            m_catalogue_map[item_id] = *item_path;
                        }
                    }
                }
            }
        }
        node = node.nextSibling();
    }
}

void EpubBook::read_spine_element(QDomNode node)
{
    node = node.firstChild();
    while (!node.isNull()) {
        if (node.isElement()) {
            if (node.nodeName() == "itemref") {
                //                qDebug() << "spine_idref:" << node.toElement().attribute("idref");
                m_spine_list.append(node.toElement().attribute("idref"));
            }
        }
        node = node.nextSibling();
    }
}

QString EpubBook::getCopyritht_content() const
{
    return m_copyritht_content;
}

EpubBook::EpubBook(QObject* parent)
    : QObject(parent)
{
    m_epub_temp_dir = QDir::tempPath();
    do {
        m_epub_temp_folder.reset(new QTemporaryDir());
        m_epub_temp_folder->setAutoRemove(true);
    } while (!m_epub_temp_folder->isValid());
}
