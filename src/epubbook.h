#ifndef EPUBBOOK_H
#define EPUBBOOK_H

#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QDomNode>
#include <QFileInfo>
#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QTemporaryDir>
#include <algorithm>

#define BUFF_SIZE 8192
#include "unzip.h"
#ifdef _WIN32
#include "iowin32.h"
#endif
#include "QCodePage437Codec.h"

#define EPUB_BOOK_RETRUN(x) {m_last_state=(x);return m_last_state;}

static QCodePage437Codec *cp437 = 0;
static const QString EPUB_SUFFIX = u8".epub";
static const QString EPUB_CONTAINER_PATH = u8"/META-INF/container.xml";
static const QString BACK_SLASH = u8"\\";
static const QString SLASH = u8"/";
static const QString OEBPS_MIMETYPE      = "application/oebps-package+xml";
static const QString EPUB_STRUCTURE_OPF = "OPF";


class EpubBook : public QObject
{
    Q_OBJECT
    enum class EPUB_STATE
    {
        ERROR_PATH_NULL,
        ERROR_FILE_MISS,
        ERROR_PATH_CORRUPT,
        ERROR_WRONG_SUFFIX,
        ERROR_CANNOT_EXTRACT,
        ERROR_EPUB_CONTAINER,
        ERROR_EPUB_CONTAINER_CONTENT,
        ERROR_EPUB_OPF_MISS,
        ERROR_EPUB_OPF_NUM,
        ERROR_EPUB_OPF,
        ERROR_UNZIP,
        OK,
    };

public:
    explicit EpubBook(QObject *parent = nullptr);
    QString get_last_error();
    EPUB_STATE open(QString &epub_path);
    QMap<QString, QString> get_the_meta_data_map() const;

    QString getCopyritht_content() const;

private:
    EPUB_STATE extract();
    EPUB_STATE process_opf();
    EPUB_STATE process_container();
    EPUB_STATE process_ncx();
    EPUB_STATE locate_copyright_page();
    QString make_target_dir();
    void read_metadata_element(QDomNode node);
    void read_manifest_element(QDomNode node);
    void read_spine_element(QDomNode node);

signals:
private:
    QString m_epub_version; //EPUB version
    QString m_unique_id;
    QString m_unique_value;
    QString m_epub_temp_dir; //the target dir for unzip epub
    QString m_epub_name;//the epub file name
    QString m_epub_book_path;//the epub file path
    QString m_epub_opf_file_path;
    EPUB_STATE m_last_state;//the epub process state
    QScopedPointer<QTemporaryDir> m_epub_temp_folder;
    QList<QString> m_epub_container_file_list;
    QMap<QString,QList<QString>> m_epub_file_structure_map;//
    QMap<QString, QString> m_id_path_map; //id和文件路径表
    QMap<QString, QString> m_id_mimetype_map; //id和文件类型表
    QMap<QString, QString> m_catalogue_map; //目录表
    QList<QString> m_spine_list; //epub内容文件阅读的顺序列表
    QMap<QString, QString> m_meta_data_map; //epub元数据表
    QString m_copyritht_content;
};

#endif // EPUBBOOK_H
