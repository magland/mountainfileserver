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

#define PRV_VERSION 0.1

void usage() {
    printf("prv sha1 [file_name]\n");
    printf("prv create [file_name|folder_name] [dst_name.prv (optional)]\n");
}

int main_sha1(QString path);
int main_create_file_prv(QString src_path,QString dst_path);
int main_create_folder_prv(QString src_path,QString dst_path);

bool should_store_content(QString file_path);
bool should_store_binary_content(QString file_path);

void print(QString str);
void println(QString str);
bool is_file(QString path);
bool is_folder(QString path);
bool write_text_file(const QString& fname, const QString& txt, QTextCodec* codec=0);
QString read_text_file(const QString& fname, QTextCodec* codec=0);
QByteArray read_binary_file(const QString& fname);

int main(int argc,char *argv[]) {
    QCoreApplication app(argc,argv);

    CLParams CLP(argc,argv);
    QString arg1=CLP.unnamed_parameters.value(0);
    QString arg2=CLP.unnamed_parameters.value(1);
    QString arg3=CLP.unnamed_parameters.value(2);

    if (arg1=="sha1") {
        QString path=arg2;
        if (path.isEmpty()) {
            usage();
            return -1;
        }
        if (!QFile::exists(path)) {
            qWarning() << "No such file: "+path;
            return -1;
        }
        return main_sha1(path);
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
            return main_create_file_prv(src_path,dst_path);
        }
        else if (is_folder(src_path)) {
            return main_create_folder_prv(src_path,dst_path);
        }
        else {
            qWarning() << "not sure why file is not a file nor a folder.";
            return -1;
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

int main_sha1(QString path) {
    QString checksum=sumit(path);
    if (checksum.isEmpty()) return -1;
    println(checksum);
    return 0;
}

QJsonObject make_file_prv_object(QString src_path) {
    QJsonObject obj;
    obj["prv_version"]=PRV_VERSION;
    obj["original_path"]=src_path;

    obj["original_checksum"]=sumit(src_path);
    obj["original_checksum_1000"]=sumit(src_path,1000);
    obj["original_size"]=QFileInfo(src_path).size();
    return obj;
}

QJsonObject make_folder_prv_object(QString src_path) {
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
                println("storing prv::::: "+src_path+"/"+file);
                obj0["prv"]=QJsonDocument::fromJson(read_text_file(src_path+"/"+file).toUtf8()).object();
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
            obj0["prv"]=make_file_prv_object(src_path+"/"+file);
        }
        files_array.push_back(obj0);
    }
    obj["files"]=files_array;

    QJsonArray folders_array;
    QStringList folder_list=QDir(src_path).entryList(QStringList("*"),QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name);
    foreach (QString folder,folder_list) {
        QJsonObject obj0;
        obj0["folder_name"]=folder;
        obj0["contents"]=make_folder_prv_object(src_path+"/"+folder);
        folders_array.push_back(obj0);
    }
    obj["folders"]=folders_array;

    return obj;
}

int main_create_file_prv(QString src_path,QString dst_path) {
    println("making file prv: "+src_path);
    QJsonObject obj=make_file_prv_object(src_path);

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

int main_create_folder_prv(QString src_path,QString dst_path) {
    QJsonObject obj=make_folder_prv_object(src_path);

    QString json=QJsonDocument(obj).toJson();
    if (write_text_file(dst_path,json)) {
        return 0;
    }
    else {
        printf("Problem writing output file.\n");
        return -1;
    }
}

bool should_store_content(QString file_path) {
    if ((file_path.endsWith(".mda"))||(file_path.endsWith(".dat"))) return false;
    return true;
}

bool should_store_binary_content(QString file_path) {
    QStringList text_file_extensions;
    text_file_extensions << "txt" << "ini" << "cfg" << "json" << "h" << "cpp" << "pro" << "sh" << "js" << "m" << "py";
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
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open file for reading: "+fname;
        return QByteArray();
    }
    QByteArray ret=file.readAll();
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
