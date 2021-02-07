#include "neteaseencode.h"
#include <QCoreApplication>

QString createSecretKey(int size)
{
    Q_UNUSED(size)
    QString res;
    QList<QChar> list = random::sample("1234567890qwertyuipasdfghjklzxcvbnm", 16);
    foreach (const QChar& ch, list)
        res.append(ch);
    return res;
}

const QString python_executable = "D:/Anaconda3/python.exe";

QHash<QString,QString> encrypted_request(const QHash<QString, QString> &dict)
{
    QString text;
    text.append('{');
    foreach (const QString& key, dict.keys()) {
        QString val = dict[key];
        text.append("\"");
        text.append(key);
        text.append("\": \"");

        text.append(val);
        text.append("\", ");
    }
    text.chop(2);//去除最后的逗号和空格
    text.append('}');
    //qDebug() << text;

    QStringList arguments;
    QString pyfile_path;
#ifdef _MSC_VER
    pyfile_path = QCoreApplication::applicationDirPath() + "/";
#endif // MSVC
    pyfile_path += "temp.py";//注意不要修改或删除temp.py
    arguments << pyfile_path << text;

    QProcess process;
    process.start(python_executable, arguments);
    process.waitForFinished();
    QString res = process.readAllStandardOutput();

    QHash<QString,QString> data;
    QStringList list = res.split("\r\n");
    if (list.size() < 2) {
        QMessageBox::warning(nullptr, __FILE__, __func__);
        return data;
    }
    QString encText = list[0];
    QString encSecKey = list[1];

    data["params"] = encText;
    data["encSecKey"] = encSecKey;
    return data;
}

QHash<QString,QString> encrypted_request(const QList<long long> &ids)
{
    QString text = "{\"csrf_token\": \"\", \"ids\": [";
    foreach (long long id, ids) {
        text.append(QString::number(id));
        text.append(", ");
    }
    text.chop(2);//去除最后的逗号和空格
    text.append("], \"br\": 999000}");
    //qDebug() << text;

    QStringList arguments;
    QString pyfile_path;
#ifdef _MSC_VER
    pyfile_path = QCoreApplication::applicationDirPath() + "/";
#endif // MSVC
    pyfile_path += "temp.py";//注意不要修改或删除temp.py
    arguments << pyfile_path << text;

    QProcess process;
    process.start(python_executable, arguments);
    process.waitForFinished();
    QString res = process.readAllStandardOutput();

    QHash<QString,QString> data;
    QStringList list = res.split("\r\n");
    if (list.size() < 2) {
        QMessageBox::warning(nullptr, __FILE__, __func__);
        return data;
    }
    QString encText = list[0];
    QString encSecKey = list[1];

    data["params"] = encText;
    data["encSecKey"] = encSecKey;
    return data;
}


QHash<QString,QString> encrypted_request2(const QHash<QString, QString> &dict)
{
    QFile file;
    QString filename;
#ifdef _MSC_VER
    filename = QCoreApplication::applicationDirPath() + "/";
#endif // MSVC
    filename += "temp2.txt";
    file.setFileName(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream out(&file);
        out << dict["s"];
        file.close();
    }

    QStringList arguments;
    QString pyfile_path;
#ifdef _MSC_VER
    pyfile_path = QCoreApplication::applicationDirPath() + "/";
#endif // MSVC
    pyfile_path += "temp2.py";//注意不要修改或删除temp2.py
    arguments << pyfile_path;

    QProcess process;
#ifdef _MSC_VER
    process.setWorkingDirectory(QCoreApplication::applicationDirPath());
#endif // MSVC
    process.start(python_executable, arguments);
    process.waitForFinished();
    QString res = process.readAllStandardOutput();

    QHash<QString,QString> data;
    QStringList list = res.split("\r\n");
    if (list.size() < 2) {
        QMessageBox::warning(nullptr, __FILE__, __func__);
        return data;
    }
    QString encText = list[0];
    QString encSecKey = list[1];

    data["params"] = encText;
    data["encSecKey"] = encSecKey;
    return data;
}

