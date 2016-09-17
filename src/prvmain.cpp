#include "clparams.h"
#include <QFile>
#include "sumit.h"
#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTime>
#include <QCoreApplication>
#include <QThread>
#include <QDir>
#include <QJsonArray>
#include <QSettings>
#include <QProcess>

#define PRV_VERSION 0.1

void usage() {
    printf("prv sha1sum [file_name]\n");
    printf("prv stat [file_name]\n");
    printf("prv create [src_file_name|folder_name] [dst_name.prv (optional)] [--create-temporary-files]\n");
    printf("prv locate [file_name.prv]\n");
    printf("prv locate --checksum=[] --checksum1000=[optional] --size=[]\n");
    printf("prv download --checksum=[] --checksum1000=[optional] --size=[]\n");
    printf("prv recover [src_file_name.prv] [dst_file_name|folder_name (optional)] \n");
    printf("prv list-subservers\n");
}

int main_sha1sum(QString path,const QVariantMap &params);
int main_stat(QString path,const QVariantMap &params);
int main_create_file_prv(QString src_path,QString dst_path,const QVariantMap &params);
int main_create_folder_prv(QString src_path,QString dst_path,const QVariantMap &params);
int main_locate_file(const QJsonObject &obj,const QVariantMap &params);
int main_download_file(const QJsonObject &obj,const QVariantMap &params);
int main_recover_file_prv(const QJsonObject &obj,QString dst_path,const QVariantMap &params);
int main_recover_folder_prv(const QJsonObject &obj,QString dst_path,const QVariantMap &params);
int main_list_subservers(const QVariantMap &params);

QString find_local_file(long size,const QString &checksum, const QString &checksum1000_optional,const QVariantMap &params);
QString find_remote_file(long size,const QString &checksum, const QString &checksum1000_optional,const QVariantMap &params);
QString find_file(long size,const QString &checksum,const QString &checksum1000_optional,const QVariantMap &params);

bool should_store_content(QString file_path);
bool should_store_binary_content(QString file_path);
QJsonObject get_config();
QString get_tmp_path();

void print(QString str);
void println(QString str);
bool is_file(QString path);
bool is_folder(QString path);
QString read_text_file(const QString& fname, QTextCodec* codec=0);
bool write_text_file(const QString& fname, const QString& txt, QTextCodec* codec=0);
QByteArray read_binary_file(const QString& fname);
bool write_binary_file(const QString& fname,const QByteArray &data);
QString make_random_id(int numchars);
QString get_http_text_curl_0(const QString& url);

int main(int argc,char *argv[]) {
    QCoreApplication app(argc,argv);

    CLParams CLP(argc,argv);
    QString arg1=CLP.unnamed_parameters.value(0);
    QString arg2=CLP.unnamed_parameters.value(1);
    QString arg3=CLP.unnamed_parameters.value(2);

    if (arg1=="sha1sum") {
        QString path=arg2;
        if (path.isEmpty()) {
            usage();
            return -1;
        }
        if (!QFile::exists(path)) {
            qWarning() << "No such file: "+path;
            return -1;
        }
        return main_sha1sum(path,CLP.named_parameters);
    }
    else if (arg1=="stat") {
        QString path=arg2;
        if (path.isEmpty()) {
            usage();
            return -1;
        }
        if (!QFile::exists(path)) {
            qWarning() << "No such file: "+path;
            return -1;
        }
        return main_stat(path,CLP.named_parameters);
    }
    else if (arg1=="create") {
        QString src_path=arg2;
        QString dst_path=arg3;
        if (src_path.isEmpty()) {
            return -1;
        }
        if (!QFile::exists(src_path)) {
            qWarning() << "No such file: "+src_path;
            return -1;
        }
        if (dst_path.isEmpty()) {
            dst_path=QFileInfo(src_path).fileName()+".prv";
        }
        if (!dst_path.endsWith(".prv")) {
            printf("Destination file must end with .prv");
            return -1;
        }
        if (is_file(src_path)) {
            return main_create_file_prv(src_path,dst_path,CLP.named_parameters);
        }
        else if (is_folder(src_path)) {
            return main_create_folder_prv(src_path,dst_path,CLP.named_parameters);
        }
        else {
            qWarning() << "not sure why file is not a file nor a folder.";
            return -1;
        }
    }
    else if ((arg1=="locate")||(arg1=="download")) {
        QJsonObject obj;
        if (CLP.named_parameters.contains("checksum")) {
            obj["original_checksum"]=CLP.named_parameters["checksum"].toString();
            obj["original_checksum_1000"]=CLP.named_parameters["checksum1000"].toString();
            obj["original_size"]=CLP.named_parameters["size"].toLongLong();
        }
        else {
            QString src_path=arg2;
            if (src_path.isEmpty()) {
                println("Source path is empty");
                return -1;
            }
            if (!QFile::exists(src_path)) {
                qWarning() << "No such file: "+src_path;
                return -1;
            }
            if (!src_path.endsWith(".prv")) {
                println("prv file must have .prv extension");
                return -1;
            }
            obj=QJsonDocument::fromJson(read_text_file(src_path).toUtf8()).object();
        }
        if (obj.contains("original_checksum")) {
            if (arg1=="locate")
                main_locate_file(obj,CLP.named_parameters);
            else
                main_download_file(obj,CLP.named_parameters);
        }
        else {
            printf("Only files can be found using 'locate' or 'download'.\n");
            return -1;
        }
    }
    else if (arg1=="list-subservers") {
        return main_list_subservers(CLP.named_parameters);
    }
    else if (arg1=="recover") {
        QString src_path=arg2;
        QString dst_path=arg3;
        if (src_path.isEmpty()) {
            return -1;
        }
        if (!QFile::exists(src_path)) {
            qWarning() << "No such file: "+src_path;
            return -1;
        }
        if (!src_path.endsWith(".prv")) {
            println("prv file must have .prv extension");
            return -1;
        }
        if (dst_path.isEmpty()) {
            QString f0=QFileInfo(src_path).fileName();
            dst_path=f0.mid(0,f0.count()-4); //remove .prv extension
        }
        QJsonObject obj=QJsonDocument::fromJson(read_text_file(src_path).toUtf8()).object();
        if (obj.contains("original_checksum")) {
            return main_recover_file_prv(obj,dst_path,CLP.named_parameters);
        }
        else {
            return main_recover_folder_prv(obj,dst_path,CLP.named_parameters);
        }
    }
    else {
        usage();
        return -1;
    }

    return 0;
}

void print(QString str) {
    printf("%s",str.toUtf8().data());
}

void println(QString str) {
    printf("%s\n",str.toUtf8().data());
}

int main_sha1sum(QString path,const QVariantMap &params) {
    Q_UNUSED(params)
    QString checksum=sumit(path);
    if (checksum.isEmpty()) return -1;
    println(checksum);
    return 0;
}

int main_stat(QString path,const QVariantMap &params) {
    Q_UNUSED(params)
    QString checksum=sumit(path);
    if (checksum.isEmpty()) return -1;
    QJsonObject obj;
    obj["checksum"]=checksum;
    obj["checksum1000"]=sumit(path,1000);
    obj["size"]=QFileInfo(path).size();
    println(QJsonDocument(obj).toJson());
    return 0;
}

QJsonObject make_file_prv_object(QString src_path,const QVariantMap &params) {
    QJsonObject obj;
    obj["prv_version"]=PRV_VERSION;
    obj["original_path"]=src_path;

    obj["original_checksum"]=sumit(src_path);
    obj["original_checksum_1000"]=sumit(src_path,1000);
    obj["original_size"]=QFileInfo(src_path).size();

    if (params.contains("create-temporary-files")) {
        QString tmp=get_tmp_path();
        if (!tmp.isEmpty()) {
            QString checksum=obj["original_checksum"].toString();
            QFile::copy(src_path,tmp+"/"+checksum+".prvdat");
        }
    }

    return obj;
}

QJsonObject make_folder_prv_object(QString src_path,const QVariantMap &params) {
    QJsonObject obj;
    obj["prv_version"]=PRV_VERSION;
    obj["original_path"]=src_path;

    QJsonArray files_array;
    QStringList file_list=QDir(src_path).entryList(QStringList("*"),QDir::Files,QDir::Name);
    foreach (QString file,file_list) {
        QJsonObject obj0;
        obj0["file_name"]=file;
        if (should_store_content(src_path+"/"+file)) {
            if (file.endsWith(".prv")) {
                QString tmp=obj0["file_name"].toString();
                obj0["file_name"]=tmp.mid(0,tmp.count()-4); //remove the .prv extension
                println("storing prv::::: "+src_path+"/"+file);
                obj0["prv"]=QJsonDocument::fromJson(read_text_file(src_path+"/"+file).toUtf8()).object();
                obj0["originally_a_prv_file"]=true;
            }
            else if (should_store_binary_content(src_path+"/"+file)) {
                println("storing binary:: "+src_path+"/"+file);
                QByteArray bytes=read_binary_file(src_path+"/"+file);
                obj0["content_base64"]=QString(bytes.toBase64());
            }
            else {
                println("storing text:::: "+src_path+"/"+file);
                obj0["content"]=read_text_file(src_path+"/"+file);
            }
        }
        else {
            println("making file prv: "+src_path+"/"+file);
            obj0["prv"]=make_file_prv_object(src_path+"/"+file,params);
        }
        files_array.push_back(obj0);
    }
    obj["files"]=files_array;

    QJsonArray folders_array;
    QStringList folder_list=QDir(src_path).entryList(QStringList("*"),QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
    foreach (QString folder,folder_list) {
        QJsonObject obj0=make_folder_prv_object(src_path+"/"+folder,params);
        obj0["folder_name"]=folder;
        folders_array.push_back(obj0);
    }
    obj["folders"]=folders_array;

    return obj;
}

int main_create_file_prv(QString src_path,QString dst_path,const QVariantMap &params) {
    println("making file prv: "+src_path);
    QJsonObject obj=make_file_prv_object(src_path,params);

    if (obj["original_checksum"].toString().isEmpty()) {
        printf("Error: checksum is empty\n");
        return -1;
    }

    QString json=QJsonDocument(obj).toJson();
    if (write_text_file(dst_path,json)) {
        return 0;
    }
    else {
        printf("Problem writing output file.\n");
        return -1;
    }
}

int main_create_folder_prv(QString src_path,QString dst_path,const QVariantMap &params) {
    QJsonObject obj=make_folder_prv_object(src_path,params);

    QString json=QJsonDocument(obj).toJson();
    if (write_text_file(dst_path,json)) {
        return 0;
    }
    else {
        printf("Problem writing output file.\n");
        return -1;
    }
}

int main_locate_file(const QJsonObject &obj,const QVariantMap &params) {
    Q_UNUSED(params)
    QString checksum=obj["original_checksum"].toString();
    QString checksum1000=obj["original_checksum_1000"].toString();
    long original_size=obj["original_size"].toVariant().toLongLong();
    QString fname_or_url=find_file(original_size,checksum,checksum1000,params);
    if (fname_or_url.isEmpty())
        return -1;
    println(fname_or_url);
    return 0;
}

int main_list_subservers(const QVariantMap &params) {
    Q_UNUSED(params)
    QJsonObject config=get_config();
    QJsonArray remote_servers=config.value("servers").toArray();
    for (int i=0; i<remote_servers.count(); i++) {
        QJsonObject server0=remote_servers[i].toObject();
        QString host=server0["host"].toString();
        int port=server0["port"].toInt();
        QString url_path=server0["path"].toString();
        QString url0=host+":"+QString::number(port)+url_path+QString("/?a=list-subservers");
        println("Connecting to "+url0);
        QString txt=get_http_text_curl_0(url0);
        print(txt+"\n\n");
    }
    return 0;
}

int main_download_file(const QJsonObject &obj,const QVariantMap &params) {
    Q_UNUSED(params)
    QString checksum=obj["original_checksum"].toString();
    QString checksum1000=obj["original_checksum_1000"].toString();
    long original_size=obj["original_size"].toVariant().toLongLong();
    QString fname_or_url=find_file(original_size,checksum,checksum1000,params);
    if (fname_or_url.isEmpty())
        return -1;

    QString cmd=QString("curl %1").arg(fname_or_url);
    return system(cmd.toUtf8().data());
}

QString find_file(QString directory,QString checksum,QString checksum1000_optional,long size,bool recursive) {
    QStringList files=QDir(directory).entryList(QStringList("*"),QDir::Files);
    foreach (QString file,files) {
        QString path=directory+"/"+file;
        if (QFileInfo(path).size()==size) {
            if (!checksum1000_optional.isEmpty()) {
                QString checksum0=sumit(path,1000);
                if (checksum0==checksum1000_optional) {
                    QString checksum1=sumit(path);
                    if (checksum1==checksum) {
                        return path;
                    }
                }
            }
            else {
                QString checksum1=sumit(path);
                if (checksum1==checksum) {
                    return path;
                }
            }
        }
    }
    if (recursive) {
        QStringList dirs=QDir(directory).entryList(QStringList("*"),QDir::Dirs|QDir::NoDotAndDotDot);
        foreach (QString dir,dirs) {
            QString path=find_file(directory+"/"+dir,checksum,checksum1000_optional,size,recursive);
            if (!path.isEmpty()) return path;
        }
    }
    return "";
}

QJsonObject get_config() {
    QString fname1=qApp->applicationDirPath()+"/../prv.json.default";
    QString fname2=qApp->applicationDirPath()+"/../config/prv.json";
    QJsonObject obj1=QJsonDocument::fromJson(read_text_file(fname1).toUtf8()).object();
    QJsonObject obj2;
    if (QFile::exists(fname2)) {
        obj2=QJsonDocument::fromJson(read_text_file(fname2).toUtf8()).object();
    }
    obj1=obj1["prv"].toObject();
    obj2=obj2["prv"].toObject();
    QStringList keys2=obj2.keys();
    foreach (QString key,keys2) {
        obj1[key]=obj2[key];
    }
    return obj1;
}

QString get_tmp_path() {
    QJsonObject config=get_config();
    QString temporary_path=config["temporary_path"].toString();
    if (temporary_path.isEmpty()) return "";
    QDir(temporary_path).mkdir("prv");
    return temporary_path+"/prv";
}

QString find_local_file(long size,const QString &checksum, const QString &checksum1000_optional,const QVariantMap &params) {
    QJsonObject config=get_config();
    QJsonArray local_search_paths0=config.value("local_search_paths").toArray();
    QStringList local_search_paths;
    for (int i=0; i<local_search_paths0.count(); i++)
        local_search_paths << local_search_paths0[0].toString();
    QString temporary_path=config.value("temporary_path").toString();
    if (!temporary_path.isEmpty()) {
        local_search_paths << temporary_path;
    }
    if (params.contains("path")) {
        local_search_paths.clear();
        local_search_paths << params["path"].toString();
    }
    for (int i=0; i<local_search_paths.count(); i++) {
        QString search_path=local_search_paths[i];
        QString fname=find_file(search_path,checksum,checksum1000_optional,size,true);
        if (!fname.isEmpty()) return fname;
    }
    return "";
}

QString make_temporary_file() {
    QString file_name=make_random_id(10)+".tmp";
    return get_tmp_path()+"/"+file_name;
}

bool curl_is_installed()
{
    QProcess P;
    P.start("curl --version");
    P.waitForStarted();
    P.waitForFinished(-1);
    int exit_code = P.exitCode();
    return (exit_code == 0);
}

QString get_http_text_curl_0(const QString& url)
{
    if (!curl_is_installed()) {
        qWarning() << "Problem in http request. It appears that curl is not installed.";
        return "";
    }
    //QString tmp_fname = make_temporary_file()+".curl";
    QString cmd = QString("curl \"%1\"").arg(url);
    //int exit_code = system(cmd.toLatin1().data());
    QProcess P;
    P.start(cmd);
    P.waitForStarted();
    P.waitForFinished(-1);
    int exit_code = P.exitCode();
    if (exit_code != 0) {
        qWarning() << "Problem with system call: " + cmd;
        //QFile::remove(tmp_fname);
        return "";
    }
    P.readAllStandardError();
    return P.readAllStandardOutput();

    /*

    QString ret = read_text_file(tmp_fname);
    QFile::remove(tmp_fname);
    return ret;
    */
}

QString find_remote_file(long size,const QString &checksum, const QString &checksum1000_optional,const QVariantMap &params) {
    Q_UNUSED(params)
    QJsonObject config=get_config();
    QJsonArray remote_servers=config.value("servers").toArray();
    for (int i=0; i<remote_servers.count(); i++) {
        QJsonObject server0=remote_servers[i].toObject();
        QString host=server0["host"].toString();
        int port=server0["port"].toInt();
        QString url_path=server0["path"].toString();
        QString url0=host+":"+QString::number(port)+url_path+QString("/?a=locate&checksum=%1&checksum1000=%2&size=%3").arg(checksum).arg(checksum1000_optional).arg(size);
        QString txt=get_http_text_curl_0(url0);
        if (!txt.isEmpty()) {
            if (!txt.contains(" ")) { //filter out error messages (good idea, or not?)
                return host+":"+QString::number(port)+url_path+"/"+txt;
            }
        }
    }
    return "";
}

QString find_file(long size,const QString &checksum,const QString &checksum1000_optional,const QVariantMap &params) {
    QString local_fname=find_local_file(size,checksum,checksum1000_optional,params);
    if (!local_fname.isEmpty()) {
        return local_fname;
    }

    if (params.contains("path")) {
        return "";
    }

    QString remote_url=find_remote_file(size,checksum,checksum1000_optional,params);
    if (!remote_url.isEmpty()) {
        return remote_url;
    }
    return "";
}

int main_recover_file_prv(const QJsonObject &obj,QString dst_path,const QVariantMap &params) {
    Q_UNUSED(params)
    QString checksum=obj["original_checksum"].toString();
    QString checksum1000=obj["original_checksum_1000"].toString();
    long original_size=obj["original_size"].toVariant().toLongLong();
    QString local_fname=find_local_file(original_size,checksum,checksum1000,params);
    if (local_fname.isEmpty()) {
        println("Unable to find local file:size="+QString::number(original_size)+" checksum="+checksum+" checksum1000="+checksum1000);
        return -1;
    }
    if (QFile::exists(dst_path)) {
        if (!QFile::remove(dst_path)) {
            qWarning() << "Unable to remove file or folder: "+dst_path;
            return -1;
        }
    }
    if (!QFile::copy(local_fname,dst_path)) {
        qWarning() << "Unable to copy file: "+local_fname+" "+dst_path;
        return -1;
    }
    return 0;
}

int main_recover_folder_prv(const QJsonObject &obj,QString dst_path,const QVariantMap &params) {
    if (QFile::exists(dst_path)) {
        println("Cannot write to directory that already exists: "+dst_path);
        return -1;
    }
    QString abs_dst_path=QDir::current().absoluteFilePath(dst_path);
    QString parent_path=QFileInfo(abs_dst_path).path();
    QString name=QFileInfo(abs_dst_path).fileName();
    if (!QDir(parent_path).mkdir(name)) {
        println("Unable to create directory. Aborting. "+abs_dst_path);
        return -1;
    }

    QJsonArray files=obj["files"].toArray();
    for (int i=0; i<files.count(); i++) {
        QJsonObject obj0=files[i].toObject();
        QString fname0=obj0["file_name"].toString();
        if (fname0.isEmpty()) {
            println("File name is empty. Aborting. "+fname0);
            return -1;
        }
        println("Recovering "+abs_dst_path+"/"+fname0);
        if (obj0.contains("content")) {
            if (!write_text_file(abs_dst_path+"/"+fname0,obj0["content"].toString())) {
                println("Unable to write file. Aborting. "+fname0);
                return -1;
            }
        }
        else if (obj0.contains("content_base64")) {
            QByteArray data0=QByteArray::fromBase64(obj0["content_base64"].toString().toUtf8());
            if (!write_binary_file(abs_dst_path+"/"+fname0,data0)) {
                println("Unable to write file. Aborting. "+fname0);
                return -1;
            }
        }
        else if (obj0.contains("prv")) {
            bool to_recover=false;
            if (params.contains("recover-all-prv-files"))
                to_recover=true;
            if (!obj0["originally_a_prv_file"].toBool())
                to_recover=true;
            if (to_recover) {
                println("**** RECOVERING .prv file: "+abs_dst_path+"/"+fname0);
                QJsonObject obj1=obj0["prv"].toObject();
                int ret=main_recover_file_prv(obj1,abs_dst_path+"/"+fname0,params);
                if (ret<0) return ret;
            }
            else {
                QString json=QJsonDocument(obj0["prv"].toObject()).toJson();
                if (!write_text_file(abs_dst_path+"/"+fname0+".prv",json)) {
                    println("Unable to write file. Aborting. "+fname0);
                    return -1;
                }
            }
        }
    }

    QJsonArray folders=obj["folders"].toArray();
    for (int i=0; i<folders.count(); i++) {
        QJsonObject obj0=folders[i].toObject();
        QString fname0=obj0["folder_name"].toString();
        if (fname0.isEmpty()) {
            println("Folder name is empty. Aborting. "+fname0);
            return -1;
        }
        int ret=main_recover_folder_prv(obj0,dst_path+"/"+fname0,params);
        if (ret<0) return ret;
    }
    return 0;
}

bool should_store_content(QString file_path) {
    if ((file_path.endsWith(".mda"))||(file_path.endsWith(".dat"))) return false;
    return true;
}

bool should_store_binary_content(QString file_path) {
    QStringList text_file_extensions;
    text_file_extensions << "txt" << "csv" << "ini" << "cfg" << "json" << "h" << "cpp" << "pro" << "sh" << "js" << "m" << "py";
    foreach (QString ext,text_file_extensions)
        if (file_path.endsWith("."+ext)) return false;
    return true;
}

bool is_file(QString path) {
    return QFileInfo(path).isFile();
}
bool is_folder(QString path) {
    return QFileInfo(path).isDir();
}

QChar make_random_alphanumeric()
{
    static int val = 0;
    val++;
    QTime time = QTime::currentTime();
    QString code = time.toString("hh:mm:ss:zzz");
    code += QString::number(qrand() + val);
    code += QString::number(QCoreApplication::applicationPid());
    code += QString::number((long)QThread::currentThreadId());
    int num = qHash(code);
    if (num < 0)
        num = -num;
    num = num % 36;
    if (num < 26)
        return QChar('A' + num);
    else
        return QChar('0' + num - 26);
}
QString make_random_id(int numchars)
{
    QString ret;
    for (int i = 0; i < numchars; i++) {
        ret.append(make_random_alphanumeric());
    }
    return ret;
}

QString read_text_file(const QString& fname, QTextCodec* codec)
{
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open file for reading: "+fname;
        return QString();
    }
    QTextStream ts(&file);
    if (codec != 0)
        ts.setCodec(codec);
    QString ret = ts.readAll();
    file.close();
    return ret;
}

QByteArray read_binary_file(const QString& fname) {
    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open file for reading: "+fname;
        return QByteArray();
    }
    QByteArray ret=file.readAll();
    file.close();
    return ret;
}

bool write_binary_file(const QString& fname,const QByteArray &data) {
    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to open file for writing: "+fname;
        return false;
    }
    bool ret=true;
    if (file.write(data)!=data.count())
        ret=false;
    file.close();
    return ret;
}

bool write_text_file(const QString& fname, const QString& txt, QTextCodec* codec)
{
    /*
     * Modification on 5/23/16 by jfm
     * We don't want an program to try to read this while we have only partially completed writing the file.
     * Therefore we now create a temporary file and then copy it over
     */

    QString tmp_fname = fname + ".tf." + make_random_id(6) + ".tmp";

    //if a file with this name already exists, we need to remove it
    //(should we really do this before testing whether writing is successful? I think yes)
    if (QFile::exists(fname)) {
        if (!QFile::remove(fname)) {
            qWarning() << "Problem in TextFile::write. Could not remove file even though it exists" << fname;
            return false;
        }
    }

    //write text to temporary file
    QFile file(tmp_fname);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Problem in TextFile::write. Could not open for writing... " << tmp_fname;
        return false;
    }
    QTextStream ts(&file);
    if (codec != 0) {
        ts.setAutoDetectUnicode(false);
        ts.setCodec(codec);
    }
    ts << txt;
    ts.flush();
    file.close();

    //check the contents of the file (is this overkill?)
    QString txt_test = read_text_file(tmp_fname, codec);
    if (txt_test != txt) {
        QFile::remove(tmp_fname);
        qWarning() << "Problem in TextFile::write. The contents of the file do not match what was expected." << fname;
        return false;
    }

    //finally, rename the file
    if (!QFile::rename(tmp_fname, fname)) {
        qWarning() << "Problem in TextFile::write. Unable to rename file at the end of the write command" << fname;
        return false;
    }

    return true;
}
