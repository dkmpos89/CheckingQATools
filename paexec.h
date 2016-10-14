#ifndef PAEXEC_H
#define PAEXEC_H

#include <QProcess>

class PaExec : public QProcess
{
public:
    static PaExec* getInstance();
    ~PaExec();
    bool init();
    void writeCmd(const QString);
    void send(const QString s);
protected slots:
  void readOctaveOutput();
private:
    explicit PaExec(QObject* parent = 0);
    static PaExec *procPExec;
};

#endif // PAEXEC_H

