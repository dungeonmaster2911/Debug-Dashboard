#ifndef CONSOLE_WINDOW_H
#define CONSOLE_WINDOW_H
#include "mainwindow.h"
#include <QTextEdit>

class Console : public QTextEdit
{
    Q_OBJECT

signals:
    void getData(const QByteArray &data);

public:
    explicit Console(QWidget *parent = nullptr);

    void putData(const QByteArray &data);
    void setLocalEchoEnabled(bool set);
    void keyPressEventProxy(QKeyEvent *e);
    void mainWindow(MainWindow* main_win){
        mw = main_win;
    }

protected:
    void keyPressEvent(QKeyEvent *e) override;
//    void mousePressEvent(QMouseEvent *e) override;
//    void mouseDoubleClickEvent(QMouseEvent *e) override;
//    void contextMenuEvent(QContextMenuEvent *e) override;

private:
    bool m_localEchoEnabled = false;
    QString msg;
    MainWindow* mw;
};

#endif // CONSOLE_H
