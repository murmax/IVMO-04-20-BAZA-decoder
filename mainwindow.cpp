#include "mainwindow.h"
#include "ui_mainwindow.h"



QMap<QString,QString> MainWindow::iniParams;
QMap<QString,QString> MainWindow::iniParamsTranslation;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QDir::setCurrent(QCoreApplication::applicationDirPath());
    loadParamsFromIni();

    if (iniParams.value("UsePostgreSQL")=="1")
    {
        bool ok;
        dbase = QSqlDatabase::addDatabase("QPSQL");
        dbase.setDatabaseName(iniParams.value("PostgreSQL_DBName")/*"myDB1"*/);
        dbase.setPort(iniParams.value("PostgreSQL_Port").toInt(&ok)/*5432*/);
        dbase.setHostName(iniParams.value("PostgreSQL_HostName")/*"localhost"*/);
        dbase.setUserName(iniParams.value("PostgreSQL_UserName")/*"User1"*/);
        dbase.setPassword(iniParams.value("PostgreSQL_UserPassword")/*"123"*/);
    } else
    {
        dbase = QSqlDatabase::addDatabase("QSQLITE");
        dbase.setDatabaseName("local.sqlite");
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_btn_browse_clicked()
{
    QString fileNames = QFileDialog::getOpenFileNames(this,"",QDir::currentPath(),"*.csv").join(";");
    ui->le_browse->setText(fileNames);
}



//INI
void MainWindow::loadParamsFromIni()
{
    QFile fileIni("config.ini");
    if (fileIni.open(QIODevice::ReadOnly))
    {
        while (!fileIni.atEnd())
        {
            QString rawLine = ((QString)fileIni.readLine()).remove(' ').remove('\n').remove('\r').remove('\t');
            int commentPosition = rawLine.indexOf('#');
            if (commentPosition!=-1)
            {
                rawLine.remove(commentPosition,rawLine.length()-commentPosition);
            }
            QStringList line = rawLine.split('=');
            if (line.length()==2)
            {
                iniParams.insert(line[0],line[1]);
            } else if (!line.isEmpty())
            {
                qDebug()<<"[ERROR] Incorrect line in .ini file";
            }
        }
        fileIni.close();
    }
    if (!checkIniParams())
    {
        restoreIniDefaults();
        //qDebug()<<"Restoring defaults";
    }
    loadParamsTranslation();
}

void MainWindow::loadParamsTranslation()
{
    QFile file(":/Translations/iniParams.txt");
    if (file.open(QIODevice::ReadOnly))
    {
        while (!file.atEnd())
        {

            QStringList strList=((QString)file.readLine()).remove('\n').split(';');
            //qDebug()<<"strList()="<<strList.length();
            if (strList.length()==2)
            {
                iniParamsTranslation.insert(strList.at(0),strList.at(1));
            }

        }
        file.close();
    }
    //qDebug()<<"iniParamsTranslation.size()="<<iniParamsTranslation.size();
}

void MainWindow::setDefaultIniFile()
{
    QFile fileIni("config.ini");
    fileIni.remove();
    QFile fileIniNew("config.ini");
    QList<QPair<QString,QString>> pairList = getIniDefaults();

    if (fileIniNew.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&fileIniNew);
        QPair<QString,QString> pair;
        for(int i=0;i<pairList.length();i++)
        {
            pair = pairList.at(i);
            stream<<pair.first<<" = "<<pair.second<<endl;
            iniParams.insert(pair.first,pair.second);
        }

        fileIniNew.close();
    }
}

void MainWindow::restoreIniDefaults()
{
    setDefaultIniFile();

    dbase.setDatabaseName(iniParams.value("PostgreSQL_DBName")/*"myDB1"*/);
    dbase.setPort(iniParams.value("PostgreSQL_Port").toInt()/*5432*/);
    dbase.setHostName(iniParams.value("PostgreSQL_HostName")/*"localhost"*/);
    dbase.setUserName(iniParams.value("PostgreSQL_UserName")/*"User1"*/);
    dbase.setPassword(iniParams.value("PostgreSQL_UserPassword")/*"123"*/);
}

QList<QPair<QString, QString> > MainWindow::getIniDefaults()
{
   return QList<QPair<QString, QString> >
        {
            QPair<QString,QString>("PostgreSQL_DBName","myDB1"),
            QPair<QString,QString>("PostgreSQL_Port","5432"),
            QPair<QString,QString>("PostgreSQL_HostName","localhost"),
            QPair<QString,QString>("PostgreSQL_UserName","User1"),
            QPair<QString,QString>("PostgreSQL_UserPassword","123"),
            QPair<QString,QString>("UsePostgreSQL","1"),
        };
}

bool MainWindow::checkIniParams()
{
    if (iniParams.isEmpty()) return false;
    QList<QPair<QString, QString> > listParams = getIniDefaults();
    QList<QPair<QString, QString> > addList;
    for (auto e : listParams)
    {
        if (!iniParams.contains(e.first))
        {
            iniParams.insert(e.first,e.second);
            addList.append(e);
        }
    }
    if (addList.length()>0){
        QFile fileIni("config.ini");
        if (fileIni.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            for (auto pair : addList){
                    QTextStream stream(&fileIni);
                    stream<<pair.first<<" = "<<pair.second<<endl;
            }
            fileIni.close();
        }
    }

    return true;
}
//~INI

//SQL

bool MainWindow::syncWithDB()
{
    //qDebug()<<"Sync";
    if (!dbase.open()) {
        qDebug() << "Что-то пошло не так!"<<dbase.lastError().text();
        //return -1;
        return false;
    }

    QSqlQuery a_query(dbase);
    QString str =
            "CREATE TABLE IF NOT EXISTS IS_table ("
                "id integer PRIMARY KEY NOT NULL, "
                "is_name VARCHAR "
            ");";
    bool b = a_query.exec(str);
    if (!b){
        qDebug()<<"Error on CREATE 1"<<a_query.lastError().text();
        return false;
    }

    if (!a_query.exec("SELECT * FROM IS_table")) {
        qDebug() << "Даже селект не получается, я пас."<<a_query.lastError().text();
        return false;
    }
    QSqlRecord rec = a_query.record();
    /*if (!a_query.next())
    {
        allUsers.append(new User("RootUser","1","Root", UserType::admin,User::maxID+1));
        allUsers.first()->root = true;
    }
    loadToDBUsers(a_query);
    loadFromDBUsers(a_query);
    loadToUsers();*/




    dbase.close();
    return true;
}
//~SQL



void MainWindow::on_btn_check_clicked()
{
    if (!syncWithDB()){

        ui->te_dbError->setText(dbase.lastError().text());
    } else
    {
        ui->te_dbError->setText("Все в порядке");
    }

}
