#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "pushpulllistform.h"
#include "qlabel.h"
#include <QMainWindow>
#include <QSerialPort>
#include <QThread>
#include<QSerialPortInfo>
#include<QScrollBar>
#include<QFile>
#include<QJsonDocument>
#include<QJsonObject>
#include<QJsonArray>
#include<QProcess>
#include<QCheckBox>
#include<QGraphicsView>
#include<QGraphicsScene>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow;
class Console;

typedef struct BtnInfo{
    QString cmd;
    QString app;
    bool shell_cmd;
    bool extWindow;
}BtnInfo;

typedef struct PushPullInfo{
    QString repo_path;
    QString push_path;
    QCheckBox *cb;
}PushPullInfo;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    class BgProcess : public QThread{
        MainWindow* main_window;
        QProcess pro;
        QString op;

    public:
        BgProcess(MainWindow* mw):main_window(mw){}
        void run() override;

        void readProcOp();
    };

    class PushProc : public QThread{
        MainWindow* main_window;
        QProcess pro;
        QString op;
        QStringList cmd;

    public:
        PushProc(MainWindow* mw,QStringList fin_cmd):main_window(mw),cmd(fin_cmd){}
        void run() override;
        void readProcOp();
        void readProcErr();
    };

    class LogcatProcess : public QThread{
        MainWindow* main_window;
        QProcess pro;
        QString op;
        QString cmd;
        QTextStream* stream;
        QFile* file;

    public:
        LogcatProcess(MainWindow* mw):main_window(mw){}
        void run() override;
        void setCmd(QString str){cmd = str;}
        void stopProc(){if(pro.isOpen()){pro.close();}}
        void readProcErr();
        void readProcOp();
    };

    class DmesgProcess : public QThread{
        MainWindow* main_window;
        QProcess pro;
        QString op;
        QString cmd;
        QTextStream* stream;
        QFile* file;

    public:
        DmesgProcess(MainWindow* mw):main_window(mw){}
        void run() override;
        void setCmd(QString str){cmd = str;}
        void stopProc(){if(pro.isOpen()){pro.close();}}
        void readProcErr();
        void readProcOp();
    };

    void setupAdbTab();

    void addButtons();

    void adbBtnPressed();

    void updateCommandOutput(QString data,QColor color = Qt::black,bool newLine=true);

    void addCustomAdbBtns(int update = 0);

    void runAdbCommand(QString name);

    void readProcOp();

    void readProcErr();
   //*************************************************************************************************************
    void setupSerialTab();

    void addCustomSerialBtns(int update = 0);

    void serialBtnPressed();

    int getBaudRate();

    void openSerial();

    void readSerial();

    void writeSerial(QString data);

    void writeSerialSplChar(char c);

    void writeSerialBtn();

    void clearSerialOp();

    void populateComPortList();

    void updateSerialOutput(QString data,QColor color = Qt::black,bool newLine=true);
    //*************************************************************************************************************


    void runCmdCommand(QString name,bool internal=0);

    //*************************************************************************************************************


    QSerialPort* sp ;

    BgProcess* bg_proc;
    LogcatProcess lg_proc;
    DmesgProcess dmesg_proc;

    QString serial_filename;
    QTextStream* serial_stream;
    QFile* serial_file;
    bool serial_file_opend;

    //************************************************************************************************************
    bool readConfigJson();
    void writeConfigJson();
    bool updateJson(QJsonDocument doc);
    void removeAdbButton();
    void addSerialButton();
    void removeSerialButton();
    void addQuickLink();
    void removeQuickLink();
    void saveRepoPath();
    void addToPushPullList();
    void removeFromPushPullList();
    //************************************************************************************************************
    void setupPushPullTab();
    void fillPushPullList(int update = 0);
    void pushBtnPressed();
    void pullBtnPressed();

    //************************************************************************************************************
    void setupQuickLinksTab(int update = 0);

    QJsonObject config_json;
private:
    bool isADBDeviceFound();
    bool isADBConnected;
    Ui::MainWindow *ui;
    PushPullListForm *fw ;
    QMap<QString,BtnInfo> adbBtns;
    QMap<QString,BtnInfo> serialBtns;
    QHash<QLabel*,QStringList> url_data;
//    QMap<QString,QString>

    QMap<QString,PushPullInfo> pushPullCheckBoxList;
    QString repo_path;
    Console *m_console = nullptr;


    QProcess pro;
};
#endif // MAINWINDOW_H
