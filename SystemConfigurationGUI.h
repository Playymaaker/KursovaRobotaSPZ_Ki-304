#pragma once

#include <windows.h>
#include <shlobj.h>
#include <winevt.h>
#include <QtWidgets/QMainWindow>
#include "ui_SystemConfigurationGUI.h"
#include <QTreeWidget>
#include <QTableWidget>
#include <QHeaderView>

#pragma comment(lib, "wevtapi.lib")
#pragma comment(lib, "shell32.lib")

class SystemConfigurationGUI : public QMainWindow
{
    Q_OBJECT

public:
    SystemConfigurationGUI(QWidget* parent = nullptr);
    ~SystemConfigurationGUI();

private:
    Ui::SystemConfigurationGUIClass ui;

    // ДОДАЙ ЦЕЙ РЯДОК:
    bool isEnglish = false;

    // Оголошення всіх функцій
    void loadRegionalSettings();
    void loadEnvironmentVariables();
    void loadControlPanelInfo();
    void loadTrashInfo();
    void loadSystemFilesInfo();
    void loadSystemFolders();
    void loadEventLogs();
    void updateTreeLanguage();
    void loadRootMenu();
};