#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QDebug>


#include <QtSql>
#include <QSqlDatabase>
#include <QTimer>
#include <QMap>
#include "QtNetwork"
#include <QUdpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


    //INI
    static QMap<QString,QString> iniParams;
    static QMap<QString,QString> iniParamsTranslation;
    void loadParamsFromIni();
    void loadParamsTranslation();
    void setDefaultIniFile();
    void restoreIniDefaults();
    QList<QPair<QString,QString>> getIniDefaults();
    bool checkIniParams();
    //~INI


    //SQL

    QSqlDatabase dbase;
    bool syncWithDB();
    //~SQL

private slots:
    void on_btn_browse_clicked();

    void on_btn_check_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
