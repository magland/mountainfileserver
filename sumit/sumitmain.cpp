#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include "sumit.h"
#include "clparams.h"

void usage() {
    printf("Usage:\n");
    printf("sumit [file_path]\n");
    printf("sumit [directory_path]\n");
    printf("sumit size [file_path]\n");
    printf("sumit find [directory_path] --checksum=[checksum] --size=[size_bytes]\n");
}

QString find_file(QString directory,QString checksum,long size,bool recursive);

int main(int argc, char *argv[])
{
    CLParams CLP(argc,argv);

    if ((CLP.unnamed_parameters.count()==1)&&(CLP.named_parameters.isEmpty())) {
        QString path=QString(argv[1]);
        if (QFileInfo(path).isDir()) {
            QString the_hash=sumit_dir(path);
            printf("%s\n",the_hash.toLatin1().data());
        }
        else {
            QString the_hash=sumit(path);
            printf("%s\n",the_hash.toLatin1().data());
        }
        return 0;
    }
    else if (
           (CLP.unnamed_parameters.count()==2)
         &&(CLP.unnamed_parameters[0]=="size")
    ) {
        QString path=CLP.unnamed_parameters[1];
        long size=QFileInfo(path).size();
        printf("%ld\n",size);
    }
    else if (
           (CLP.unnamed_parameters.count()==2)
         &&(CLP.unnamed_parameters[0]=="stat")
    ) {
        QString path=CLP.unnamed_parameters[1];
        long size=QFileInfo(path).size();
        QString the_hash=sumit(path);
        QJsonObject resp;
        resp["size"]=(long long)size;
        resp["checksum"]=the_hash;
        QString resp_json=QJsonDocument(resp).toJson();
        printf("%s\n",resp_json.toUtf8().data());
    }
    else if (
           (CLP.unnamed_parameters.count()==2)
         &&(CLP.unnamed_parameters[0]=="find")
         &&(CLP.named_parameters.contains("checksum"))
         &&(CLP.named_parameters.contains("size"))
    ) {
        QString path=find_file(CLP.unnamed_parameters[1],CLP.named_parameters["checksum"].toString(),CLP.named_parameters["size"].toLongLong(),true);
        printf("%s\n",path.toUtf8().data());
    }
    else {
        usage();
        return -1;
    }
}

QString find_file(QString directory,QString checksum,long size,bool recursive) {
    QStringList files=QDir(directory).entryList(QStringList("*"),QDir::Files);
    foreach (QString file,files) {
        QString path=directory+"/"+file;
        if (QFileInfo(path).size()==size) {
            QString checksum0=sumit(path);
            if (checksum0==checksum) {
                return path;
            }
        }
    }
    if (recursive) {
        QStringList dirs=QDir(directory).entryList(QStringList("*"),QDir::Dirs|QDir::NoDotAndDotDot);
        foreach (QString dir,dirs) {
            QString path=find_file(directory+"/"+dir,checksum,size,recursive);
            if (!path.isEmpty()) return path;
        }
    }
    return "";
}
