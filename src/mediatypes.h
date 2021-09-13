#ifndef MEDIATYPES_H
#define MEDIATYPES_H
#include <QCoreApplication>
#include <QHash>

class MediaTypes
{
    Q_DECLARE_TR_FUNCTIONS(MediaTypes)

public:
    static MediaTypes* instance();
    QString GetMediaTypeFromExtension(const QString& extension, const QString& fallback = "");
    QString GetGroupFromMediaType(const QString& mediatype, const QString& fallback = "");
    QString GetResourceDescFromMediaType(const QString& mediatype, const QString& fallback = "");

private:
    MediaTypes();
    void SetExtToMTypeMap();
    void SetMTypeToGroupMap();

    void SetMTypeToRDescMap();

    QHash<QString, QString> m_ExtToMType;

    QHash<QString, QString> m_MTypeToGroup;

    QHash<QString, QString> m_MTypeToRDesc;

    static MediaTypes* m_instance;
};

#endif // MEDIATYPES_H
