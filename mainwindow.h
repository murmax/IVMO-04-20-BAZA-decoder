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
    int rowsAppliedCorrect=0;
    int rowsAppliedIncorrect=0;

    //~SQL

    QStringList parseRow(QString row, QSqlQuery query);
private slots:
    void on_btn_browse_clicked();

    void on_btn_check_clicked();

    void on_btn_parse_clicked();

private:
    Ui::MainWindow *ui;
    QStringList parseRowTypeFirst(QString row, QSqlQuery query);
    QStringList parseRowTypeSecond(QString row, QSqlQuery query);

    int selectUnique(QString tableName, QString paramName,QString value, QSqlQuery query, QMap<QString,QString> additionalParams = QMap<QString,QString>());

    QStringList firstLineHeader;
    QStringList secondLineHeader;
    int findClosestInHeader(bool firstHeader, QString value,int defaultValue=-1, int startAt=0, int endAt=-1);
};
#endif // MAINWINDOW_H
