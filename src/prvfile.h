/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 9/21/2016
*******************************************************/
#ifndef PRVFILE_H
#define PRVFILE_H

#define PRV_VERSION 0.1

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

struct PrvFileCreateOptions {
    bool create_temporary_files=false;
};

struct PrvFileLocateOptions {
    QStringList local_search_paths;
    bool search_remotely=false;
    QJsonArray remote_servers;
};

struct PrvFileRecoverOptions {
    bool recover_all_prv_files=false;
    PrvFileLocateOptions locate_opts;
};

class PrvFilePrivate;
class PrvFile
{
public:
    friend class PrvFilePrivate;
    PrvFile(const QString &file_path="");
    PrvFile(const QJsonObject &obj);
    virtual ~PrvFile();
    QJsonObject object() const;
    bool read(const QString &file_path);
    bool write(const QString &file_path) const;
    bool representsFile() const;
    bool representsFolder() const;
    bool createFromFile(const QString &file_path,const PrvFileCreateOptions &opts);
    bool createFromFolder(const QString &folder_path,const PrvFileCreateOptions &opts);
    bool recoverFile(const QString &dst_file_path,const PrvFileRecoverOptions &opts);
    bool recoverFolder(const QString &dst_folder_path,const PrvFileRecoverOptions &opts);
    QString locate(const PrvFileLocateOptions &opts);
private:
    PrvFilePrivate *d;
};

QString read_text_file(const QString& fname, QTextCodec* codec=0);
bool write_text_file(const QString& fname, const QString& txt, QTextCodec* codec=0);
QString make_random_id(int numchars);
QString http_get_text_curl_0(const QString& url);
QString http_post_file_curl_0(const QString& url,const QString &filename);
bool is_url(QString txt);

#endif // PRVFILE_H
