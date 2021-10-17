#include "check.h"

bool is_binary(const QString& filename)
{
    // https://eli.thegreenplace.net/2011/10/19/perls-guess-if-file-is-text-or-binary-implemented-in-python/
    QStringList binary_extensions = {"pyc", "iso", "zip", "pdf"};
    foreach (QString ext, binary_extensions) {
        if (filename.endsWith(ext))
            return true;
    }

    QFileInfo info(filename);
    if (!info.isFile())
        return true;//不是文件，比如文件夹我们认为是二进制类型
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    QByteArray chunk = file.read(1024);
    file.close();

    if (chunk.isEmpty())
        return false;
    QList<int> text_characters = {'\n', '\r', '\t', '\f', '\b'};
    for (int i=32;i<127;i++)
        text_characters.append(i);
    //QList<int> nontext;
	int nontextCnt = 0;
    foreach (char ch, chunk) {
        if (ch == 0)
            // Files with null bytes are binary
            return true;
		if (!text_characters.contains(ch))
		{
			//nontext.append(ch);
			nontextCnt++;
		}  
    }
    //只能针对ASCII编码，对utf-8编码会误判
    return static_cast<double>(nontextCnt) / chunk.size() > 0.3;
}
