#pragma execution_character_set("utf-8")
#include "SystemConfigurationGUI.h"
#include <vector>
#include <string>
#include <algorithm>
#include <QStyle>
#include <QApplication>
#include <QListWidget>
#include <QListWidgetItem>
#include <QProcess>
#include <QFile>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QMenu>
#include <QFileDialog>
#include <QClipboard>
#include <QTextStream>
#include <QDateTime>
#include <QIcon>
#include <shlwapi.h>
#include <map>
#include <QMessageBox> 
#pragma comment(lib, "shlwapi.lib")


// ==============================================================================
// ГЛОБАЛЬНИЙ БЛОК (Алгоритм для отримання інстальованих мов
// ==============================================================================
struct LanguagePackage {
    QString lcidStr;
    QString name;
    bool isActive;
};

static std::vector<LanguagePackage>* g_installedLangs = nullptr;

BOOL CALLBACK EnumLocalesProcQt(LPWSTR lpLocaleString) {
    if (!g_installedLangs) return FALSE;

    LCID lcid = wcstoul(lpLocaleString, nullptr, 16);

    ULONG numLangs = 0, bufferLen = 0;
    GetSystemPreferredUILanguages(MUI_LANGUAGE_NAME, &numLangs, NULL, &bufferLen);
    std::wstring langBuffer(bufferLen, L'\0');
    GetSystemPreferredUILanguages(MUI_LANGUAGE_NAME, &numLangs, &langBuffer[0], &bufferLen);

    wchar_t currentName[LOCALE_NAME_MAX_LENGTH];
    LCIDToLocaleName(lcid, currentName, LOCALE_NAME_MAX_LENGTH, 0);

    if (langBuffer.find(currentName) != std::wstring::npos) {
        wchar_t nameBuffer[256] = { 0 };
        if (GetLocaleInfoW(lcid, LOCALE_SLOCALIZEDDISPLAYNAME, nameBuffer, 256)) {
            LanguagePackage pkg;

            pkg.lcidStr = QString::asprintf("LCID %04Xh", lcid);
            pkg.name = QString::fromWCharArray(nameBuffer);
            pkg.isActive = (lcid == GetUserDefaultUILanguage());

            auto it = std::find_if(g_installedLangs->begin(), g_installedLangs->end(),
                [&](const LanguagePackage& p) { return p.name == pkg.name; });

            if (it == g_installedLangs->end()) {
                g_installedLangs->push_back(pkg);
            }
        }
    }
    return TRUE;
}
// ==============================================================================

SystemConfigurationGUI::SystemConfigurationGUI(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    ui.treeWidget->setMaximumWidth(250);
    ui.treeWidget->setHeaderHidden(true);
    this->resize(800, 500);
    this->setWindowIcon(QIcon("SystemConfigurationGUI.ico"));
    ui.tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.tableWidget->setSortingEnabled(true);
    ui.tableWidget->verticalHeader()->setVisible(false);
    ui.tableWidget->setShowGrid(false);
    updateTreeLanguage();
    // Встановлюємо початкові заголовки 
    ui.tableWidget->setColumnCount(2);
    ui.tableWidget->setHorizontalHeaderLabels({ QString::fromUtf8("Властивість"), QString::fromUtf8("Значення") });
    ui.tableWidget->horizontalHeader()->setStretchLastSection(true);


// ==========================================================================
// ЛОГІКА МЕНЮ
// ==========================================================================

// ==========================================================================
    // Кнопка "Довідка" 
    // ==========================================================================
    connect(ui.action_Help, &QAction::triggered, this, [this]() {
        QString title = isEnglish ? "About Program & Help" : QString::fromUtf8("Про програму та Довідка");

        QString text;
        if (isEnglish) {
            text = "<h2>System Configuration Utility v1.0</h2>"
                "<p><b>Developer:</b> Student Roman Gulchenko (Group KI-304)</p>"
                "<hr>"
                "<h3>User Manual:</h3>"
                "<ul>"
                "<li><b>Navigation:</b> Use the left-side tree structure or click the large graphical icons on the main page to switch between different system configuration sections.</li>"
                "<li><b>Settings:</b> Use the <b>Settings</b> menu at the top to toggle the stylish <b>Dark Theme</b> or keep the application <b>Always on Top</b> of other windows.</li>"
                "<li><b>Table Context Menu:</b> Right-click any row in the data table to quickly <b>Copy</b> specific values, copy the entire table, or <b>Refresh</b> the current data.</li>"
                "<li><b>Double-Click Actions:</b>"
                "  <ul>"
                "    <li><i>Regional Settings:</i> Opens the native Windows Region settings panel.</li>"
                "    <li><i>Control Panel:</i> Directly launches the selected Windows Control Panel tool.</li>"
                "    <li><i>Recycle Bin:</i> Triggers a secure emptying command with a standard system confirmation dialog.</li>"
                "    <li><i>System Files:</i> Instantly opens configuration files (like .ini or hosts) in Notepad.</li>"
                "    <li><i>System Folders:</i> Opens the selected Windows directory path in File Explorer.</li>"
                "    <li><i>Event Logs:</i> Opens a custom properties dialog displaying the complete detailed log description.</li>"
                "  </ul>"
                "</li>"
                "<li><b>Reports (Right-Click):</b> Right-click any item in the tree view to open the <b>Quick Report</b> menu. You can export data into beautifully formatted Plain Text (.txt), styled HTML (.html), or MHTML web archives (.mht). Selecting the main <b>Configuration</b> node generates a comprehensive full-system report.</li>"
                "</ul>";
        }
        else {
            text = QString::fromUtf8(
                "<h2>Утиліта конфігурації системи v1.0</h2>"
                "<p><b>Розробник:</b> Студент Гульченко Роман (Група КІ-304)</p>"
                "<hr>"
                "<h3>Інструкція користувача:</h3>"
                "<ul>"
                "<li><b>Навігація:</b> Використовуйте бічне дерево розділів або натискайте на великі графічні ярлики на головній сторінці для швидкого перемикання між категоріями системи.</li>"
                "<li><b>Налаштування:</b> Використовуйте меню <b>Налаштування</b> для увімкнення стильної <b>Темної теми</b> або закріплення програми <b>Поверх інших вікон</b>.</li>"
                "<li><b>Меню таблиці:</b> Натисніть правою кнопкою миші на будь-який рядок у таблиці, щоб швидко <b>Копіювати</b> окремі значення, всю таблицю, або <b>Оновити</b> поточні дані.</li>"
                "<li><b>Дії по подвійному кліку:</b>"
                "  <ul>"
                "    <li><i>Регіональні установки:</i> Відкриває рідне системне вікно налаштувань регіону Windows.</li>"
                "    <li><i>Панель керування:</i> Запускає відповідний аплет Панелі керування Windows.</li>"
                "    <li><i>Кошик:</i> Викликає команду очищення кошика зі стандартним системним запитом на підтвердження.</li>"
                "    <li><i>Системні файли:</i> Відкриває конфігураційні файли (наприклад, .ini або hosts) безпосередньо в Блокноті.</li>"
                "    <li><i>Системні папки:</i> Відкриває обрану директорію у Провіднику Windows.</li>"
                "    <li><i>Протоколи подій:</i> Відкриває окреме зручне вікно з повним детальним описом обраної помилки чи події.</li>"
                "  </ul>"
                "</li>"
                "<li><b>Генерація звітів (Права кнопка миші):</b> Натисніть правою кнопкою миші на будь-який пункт дерева, щоб викликати меню <b>«Швидкий звіт»</b>. Ви можете експортувати дані у Простий текст (.txt), табличний HTML (.html) з оформленням або єдиний веб-архів MHTML (.mht). Обрання головного вузла <b>«Конфігурація»</b> автоматично згенерує повний звіт по всіх розділах програми.</li>"
                "</ul>"
            );
        }

        QMessageBox::about(this, title, text);
        });


    // ==========================================================================
    // НАЛАШТУВАННЯ: ТЕМНА ТЕМА
    // ==========================================================================
    static bool isDarkTheme = false;
    connect(ui.action_DarkTheme, &QAction::triggered, this, [this]() {
        isDarkTheme = !isDarkTheme; 

        if (isDarkTheme) {
            qApp->setStyleSheet(
                // 1. ГЛОБАЛЬНЕ ПРАВИЛО: змушуємо весь текст бути світлим
                "QWidget { color: #e0e0e0; }"

                // 2. Головні вікна
                "QMainWindow, QDialog, QMessageBox { background-color: #1e1e1e; }"

                // 3. Таблиці та списки
                "QTreeWidget, QTableWidget, QListWidget, QTextEdit { background-color: #252526; color: #e0e0e0; border: 1px solid #3f3f46; selection-background-color: #007acc; }"
                "QTableWidget::item { background-color: #252526; color: #e0e0e0; }"
                "QTableWidget::item:selected { background-color: #007acc; color: #ffffff; }"
                "QHeaderView::section { background-color: #333333; color: #ffffff; padding: 5px; border: 1px solid #1e1e1e; }"

                // 4. Жорсткий стиль для МЕНЮ (Довідка, Налаштування)
                "QMenuBar { background-color: #1e1e1e; color: #ffffff; }"
                "QMenuBar::item { background-color: transparent; color: #ffffff; }"
                "QMenuBar::item:selected { background-color: #333333; color: #ffffff; }"
                "QMenuBar::item:pressed { background-color: #333333; color: #ffffff; }"

                "QMenu { background-color: #1e1e1e; color: #ffffff; border: 1px solid #454545; }"
                "QMenu::item { background-color: transparent; color: #ffffff; padding: 5px 20px; }"
                "QMenu::item:selected { background-color: #007acc; color: #ffffff; }"
            );
            ui.action_DarkTheme->setText(isEnglish ? "Light Theme" : QString::fromUtf8("Світла тема"));
        }
        else {
            // Повертаємо стандартну системну тему Windows
            qApp->setStyleSheet("");
            ui.action_DarkTheme->setText(isEnglish ? "Dark Theme" : QString::fromUtf8("Темна тема"));
        }
        });

    // ==========================================================================
    // НАЛАШТУВАННЯ: ПОВЕРХ ІНШИХ ВІКОН
    // ==========================================================================
    connect(ui.action_AlwaysOnTop, &QAction::triggered, this, [this](bool checked) {
        Qt::WindowFlags flags = this->windowFlags();
        if (checked) {
            // Додаємо прапорець "Завжди зверху"
            this->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
        }
        else {
            // Прибираємо прапорець "Завжди зверху"
            this->setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
        }

        this->show();

        if (isEnglish) {
            ui.action_AlwaysOnTop->setText(checked ? "On Top: ON" : "Always on Top");
        }
        else {
            ui.action_AlwaysOnTop->setText(checked ? QString::fromUtf8("Поверх вікон: ТАК") : QString::fromUtf8("Поверх інших вікон"));
        }
        });

    // Кнопка "Зміна мови"
    connect(ui.action_Language, &QAction::triggered, this, [this]() {
        QString title = isEnglish ? "Language Selection" : QString::fromUtf8("Вибір мови");
        QString question = isEnglish ?
            "Switch to Ukrainian?" :
            QString::fromUtf8("Переключити інтерфейс на англійську?");

        QMessageBox::StandardButton reply = QMessageBox::question(this, title, question, QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            isEnglish = !isEnglish;

            // 1. Оновлюємо дерево
            updateTreeLanguage();

            // 2. Очищаємо таблицю, щоб користувач обрав пункт заново і дані оновилися
            ui.tableWidget->setRowCount(0);

            // 3. Оновлюємо заголовки таблиці 
            if (isEnglish) {
                ui.tableWidget->setHorizontalHeaderLabels({ "Property / Item", "Value / Path" });
            }
            else {
                ui.tableWidget->setHorizontalHeaderLabels({ QString::fromUtf8("Властивість / Елемент"), QString::fromUtf8("Значення / Опис") });
            }

            // === 4. ОНОВЛЮЄМО НАЗВИ МЕНЮ ТА СТАРИХ КНОПОК ===
            ui.menu->setTitle(isEnglish ? "Settings" : QString::fromUtf8("Налаштування"));

            ui.action_Language->setText(isEnglish ? "Change Language" : QString::fromUtf8("Зміна мови"));
            ui.action_Help->setText(isEnglish ? "Help & About" : QString::fromUtf8("Довідка"));

            // === 5. ОНОВЛЮЄМО ПЕРЕКЛАД НОВИХ КНОПОК НАЛАШТУВАНЬ ===
            ui.action_DarkTheme->setText(isDarkTheme ?
                (isEnglish ? "Light Theme" : QString::fromUtf8("Світла тема")) :
                (isEnglish ? "Dark Theme" : QString::fromUtf8("Темна тема")));

            bool isOnTop = ui.action_AlwaysOnTop->isChecked();
            if (isEnglish) {
                ui.action_AlwaysOnTop->setText(isOnTop ? "On Top: ON" : "Always on Top");
            }
            else {
                ui.action_AlwaysOnTop->setText(isOnTop ? QString::fromUtf8("Поверх вікон: ТАК") : QString::fromUtf8("Поверх інших вікон"));
            }
            // ======================================================

            QMessageBox::information(this,
                isEnglish ? "Language" : QString::fromUtf8("Мова"),
                isEnglish ? "Language set to English" : QString::fromUtf8("Мову змінено на українську"));
        }
        });

    // ==========================================================================
        // ЛОГІКА ТАБЛИЦІ (Контекстне меню: Копіювати / Оновити)
        // ==========================================================================

        // 1. Дозволяємо таблиці викликати наше власне меню
    ui.tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    // 2. Обробляємо клік правою кнопкою миші по таблиці
    connect(ui.tableWidget, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos) {

        QMenu contextMenu(this);

        // Створюємо пункти меню з підтримкою двох мов
        QAction* actionCopy = contextMenu.addAction(isEnglish ? "Copy" : QString::fromUtf8("Копіювати"));
        QAction* actionCopyAll = contextMenu.addAction(isEnglish ? "Copy All" : QString::fromUtf8("Копіювати всі"));
        QAction* actionCopyValue = contextMenu.addAction(isEnglish ? "Copy Value" : QString::fromUtf8("Копіювати значення"));

        contextMenu.addSeparator(); 

        QAction* actionRefresh = contextMenu.addAction(isEnglish ? "Refresh" : QString::fromUtf8("Оновити"));

        QAction* selectedAction = contextMenu.exec(ui.tableWidget->viewport()->mapToGlobal(pos));

        if (!selectedAction) return; 

        // Отримуємо доступ до буфера обміну Windows
        QClipboard* clipboard = QApplication::clipboard();

        // ОБРОБКА ДІЙ
        if (selectedAction == actionCopy) {
            QTableWidgetItem* item = ui.tableWidget->currentItem();
            if (item) {
                int row = item->row();
                QString prop = ui.tableWidget->item(row, 0) ? ui.tableWidget->item(row, 0)->text() : "";
                QString val = ui.tableWidget->item(row, 1) ? ui.tableWidget->item(row, 1)->text() : "";
                clipboard->setText(prop + "\t" + val); // Копіюємо рядок через табуляцію
            }
        }
        else if (selectedAction == actionCopyValue) {
            QTableWidgetItem* item = ui.tableWidget->currentItem();
            if (item) {
                int row = item->row();
                QString val = ui.tableWidget->item(row, 1) ? ui.tableWidget->item(row, 1)->text() : "";
                clipboard->setText(val);
            }
        }
        else if (selectedAction == actionCopyAll) {
            QString allText;
            int rows = ui.tableWidget->rowCount();
            int cols = ui.tableWidget->columnCount();

            for (int r = 0; r < rows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    if (!ui.tableWidget->isColumnHidden(c)) {
                        QTableWidgetItem* cell = ui.tableWidget->item(r, c);
                        allText += (cell ? cell->text() : "") + "\t";
                    }
                }
                allText += "\n";
            }
            clipboard->setText(allText); // Копіюємо всю таблицю
        }
        else if (selectedAction == actionRefresh) {
            // Щоб оновити дані, ми просто симулюємо клік по поточному активному пункту в дереві зліва
            QTreeWidgetItem* currentTreeItem = ui.treeWidget->currentItem();
            if (currentTreeItem) {
                emit ui.treeWidget->itemClicked(currentTreeItem, 0);
            }
        }
        });

    // ==========================================================================
    // ЛОГІКА ДЕРЕВА (Контекстне меню по правій кнопці миші)
    // ==========================================================================

    // 1. Обов'язково кажемо дереву, що воно має викликати наше власне меню
    ui.treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    // 2. Обробляємо сигнал натискання правої кнопки
    connect(ui.treeWidget, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        // Отримуємо елемент, по якому клікнули
        QTreeWidgetItem* item = ui.treeWidget->itemAt(pos);
        if (!item) return; // Якщо клікнули на порожнє місце - нічого не робимо

        // Створюємо головне меню
        QMenu contextMenu(this);

        // Створюємо підменю "Швидкий звіт"
        QMenu* reportMenu = contextMenu.addMenu(isEnglish ? "Quick Report" : QString::fromUtf8("Швидкий звіт"));

        // Додаємо стандартну системну іконку документа до підменю
        reportMenu->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));

        // Додаємо пункти у підменю
        QAction* actionText = reportMenu->addAction(isEnglish ? "Plain text" : QString::fromUtf8("Простий текст"));
        QAction* actionHtml = reportMenu->addAction("HTML");
        QAction* actionMhtml = reportMenu->addAction("MHTML");

        // Показуємо меню там, де знаходиться курсор миші і чекаємо вибору
        QAction* selectedAction = contextMenu.exec(ui.treeWidget->viewport()->mapToGlobal(pos));

        // Перевіряємо, що саме вибрав користувач
        if (selectedAction == actionText) {
            QString fileName = QFileDialog::getSaveFileName(this,
                isEnglish ? "Save Report" : QString::fromUtf8("Зберегти звіт"),
                "", "Text Files (*.txt);;All Files (*)");

            if (!fileName.isEmpty()) {
                QFile file(fileName);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&file);

                    out << "======================================================\n";
                    out << (isEnglish ? " REPORT: " : QString::fromUtf8(" ЗВІТ: ")) << item->text(0) << "\n";
                    out << (isEnglish ? " DATE:   " : QString::fromUtf8(" ДАТА:   ")) << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
                    out << "======================================================\n\n";

                    // РОЗУМНА МІНІ-ФУНКЦІЯ: Читає таблицю, вирівнює текст і додає заголовки колонок
                    auto writeTableToFile = [&]() {
                        int rows = ui.tableWidget->rowCount();
                        int cols = ui.tableWidget->columnCount();
                        if (rows == 0) return; // Якщо даних немає, нічого не робимо

                        int lastVisCol = cols - 1;
                        while (lastVisCol >= 0 && ui.tableWidget->isColumnHidden(lastVisCol)) lastVisCol--;

                        std::vector<int> colWidths(cols, 15);
                        for (int c = 0; c < cols; ++c) {
                            if (ui.tableWidget->isColumnHidden(c)) continue;

                            // Враховуємо ширину заголовка
                            QTableWidgetItem* hItem = ui.tableWidget->horizontalHeaderItem(c);
                            if (hItem && hItem->text().length() > colWidths[c]) colWidths[c] = hItem->text().length();

                            // Враховуємо ширину тексту в комірках
                            for (int r = 0; r < rows; ++r) {
                                QTableWidgetItem* cell = ui.tableWidget->item(r, c);
                                if (cell && cell->text().length() > colWidths[c] && cell->text().length() < 50) {
                                    colWidths[c] = cell->text().length();
                                }
                            }
                        }

                        // ДРУКУЄМО ЗАГОЛОВКИ ТАБЛИЦІ
                        for (int c = 0; c < cols; ++c) {
                            if (!ui.tableWidget->isColumnHidden(c)) {
                                QString hText = ui.tableWidget->horizontalHeaderItem(c) ? ui.tableWidget->horizontalHeaderItem(c)->text() : "";
                                if (c == lastVisCol) out << hText;
                                else out << hText.leftJustified(colWidths[c] + 3, ' ');
                            }
                        }
                        out << "\n";

                        // ДРУКУЄМО ЛІНІЮ ПІД ЗАГОЛОВКАМИ (----------------)
                        for (int c = 0; c < cols; ++c) {
                            if (!ui.tableWidget->isColumnHidden(c)) {
                                QString line(c == lastVisCol ? 20 : colWidths[c], '-');
                                if (c == lastVisCol) out << line;
                                else out << line.leftJustified(colWidths[c] + 3, ' ');
                            }
                        }
                        out << "\n";

                        // ДРУКУЄМО САМІ ДАНІ
                        for (int r = 0; r < rows; ++r) {
                            for (int c = 0; c < cols; ++c) {
                                if (!ui.tableWidget->isColumnHidden(c)) {
                                    QTableWidgetItem* cell = ui.tableWidget->item(r, c);
                                    QString text = cell ? cell->text() : "";
                                    text.replace("\n", " "); text.replace("\r", "");

                                    if (c == lastVisCol) out << text;
                                    else {
                                        if (text.length() > colWidths[c]) text = text.left(colWidths[c] - 3) + "...";
                                        out << text.leftJustified(colWidths[c] + 3, ' ');
                                    }
                                }
                            }
                            out << "\n";
                        }
                        out << "\n\n";
                        }; 

                    // ПЕРЕВІРКА: Клікнули на "Конфігурація" чи на конкретну вкладку?
                    if (item->parent() == nullptr) {
                        // РОБИМО ПОВНИЙ ЗВІТ УСІЄЇ СИСТЕМИ
                        for (int i = 0; i < item->childCount(); ++i) {
                            QTreeWidgetItem* child = item->child(i);

                            out << ">>> " << child->text(0).toUpper() << " <<<\n";
                            out << "------------------------------------------------------\n";

                            // ВИКЛИКАЄМО ФУНКЦІЇ БЕЗПОСЕРЕДНЬО 
                            switch (i) {
                            case 0: loadRegionalSettings(); break;
                            case 1: loadEnvironmentVariables(); break;
                            case 2: loadControlPanelInfo(); break;
                            case 3: loadTrashInfo(); break;
                            case 4: loadSystemFilesInfo(); break;
                            case 5: loadSystemFolders(); break;
                            case 6: loadEventLogs(); break;
                            }
                            writeTableToFile(); // Пишемо у файл те, що функція щойно завантажила
                        }

                        // Після завершення повертаємося на головний екран
                        ui.tableWidget->setRowCount(0);
                        ui.stackedWidget->setCurrentIndex(1);
                        loadRootMenu();
                    }
                    else {
                        // РОБИМО ЗВІТ ЛИШЕ ДЛЯ ОДНІЄЇ ВИБРАНОЇ ВКЛАДКИ
                        int index = item->parent()->indexOfChild(item);
                        switch (index) {
                        case 0: loadRegionalSettings(); break;
                        case 1: loadEnvironmentVariables(); break;
                        case 2: loadControlPanelInfo(); break;
                        case 3: loadTrashInfo(); break;
                        case 4: loadSystemFilesInfo(); break;
                        case 5: loadSystemFolders(); break;
                        case 6: loadEventLogs(); break;
                        }
                        writeTableToFile();
                    }

                    file.close();
                    QMessageBox::information(this,
                        isEnglish ? "Success" : QString::fromUtf8("Успіх"),
                        isEnglish ? "Report saved!" : QString::fromUtf8("Звіт успішно збережено!"));
                }
            }
        }
else if (selectedAction == actionHtml) {
    // 1. Відкриваємо вікно збереження
    QString fileName = QFileDialog::getSaveFileName(this,
        isEnglish ? "Save HTML Report" : QString::fromUtf8("Зберегти HTML звіт"),
        "", "HTML Files (*.html *.htm);;All Files (*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);



            // 2. Пишемо базовий каркас HTML-сторінки та красиві стилі (CSS)
            out << "<!DOCTYPE html>\n<html>\n<head>\n";
            out << "<meta charset='UTF-8'>\n";
            out << "<title>" << (isEnglish ? "System Report" : QString::fromUtf8("Системний звіт")) << "</title>\n";
            out << "<style>\n";
            out << "body { font-family: 'Segoe UI', Tahoma, Arial, sans-serif; background-color: #f4f6f9; color: #333; margin: 20px; }\n";
            out << "h1 { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 10px; }\n";
            out << "h2 { color: #2980b9; margin-top: 30px; }\n";
            out << ".report-header { background-color: #fff; padding: 15px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-bottom: 20px; }\n";
            out << "table { width: 100%; border-collapse: collapse; margin-top: 10px; background-color: #fff; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }\n";
            out << "th { background-color: #34495e; color: #fff; padding: 12px; text-align: left; border: 1px solid #ddd; }\n";
            out << "td { padding: 10px 12px; border: 1px solid #ddd; }\n";
            out << "tr:nth-child(even) { background-color: #f9f9f9; }\n";
            out << "tr:hover { background-color: #f1f2f6; }\n";
            out << "</style>\n";
            out << "</head>\n<body>\n";

            // 3. Шапка звіту
            out << "<div class='report-header'>\n";
            out << "<h1>" << (isEnglish ? "System Configuration Report" : QString::fromUtf8("Звіт конфігурації системи")) << "</h1>\n";
            out << "<p><b>" << (isEnglish ? "Target:" : QString::fromUtf8("Ціль:")) << "</b> " << item->text(0) << "</p>\n";
            out << "<p><b>" << (isEnglish ? "Date generated:" : QString::fromUtf8("Дата створення:")) << "</b> "
                << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "</p>\n";
            out << "</div>\n";

            // 4. МІНІ-ФУНКЦІЯ для запису таблиці в HTML
            auto writeTableToHtml = [&]() {
                int rows = ui.tableWidget->rowCount();
                int cols = ui.tableWidget->columnCount();
                if (rows == 0) return;

                out << "<table>\n";

                // Заголовки (th)
                out << "<tr>\n";
                for (int c = 0; c < cols; ++c) {
                    if (!ui.tableWidget->isColumnHidden(c)) {
                        QString hText = ui.tableWidget->horizontalHeaderItem(c) ? ui.tableWidget->horizontalHeaderItem(c)->text() : "";
                        out << "<th>" << hText.toHtmlEscaped() << "</th>\n";
                    }
                }
                out << "</tr>\n";

                // Дані (td)
                for (int r = 0; r < rows; ++r) {
                    out << "<tr>\n";
                    for (int c = 0; c < cols; ++c) {
                        if (!ui.tableWidget->isColumnHidden(c)) {
                            QTableWidgetItem* cell = ui.tableWidget->item(r, c);
                            QString text = cell ? cell->text() : "";

                            // toHtmlEscaped захищає від проблем, якщо в тексті є символи < або >
                            out << "<td>" << text.toHtmlEscaped() << "</td>\n";
                        }
                    }
                    out << "</tr>\n";
                }
                out << "</table>\n";
                };

            // 5. Логіка генерації (Повний або Частковий звіт)
            if (item->parent() == nullptr) {
                // ПОВНИЙ ЗВІТ
                for (int i = 0; i < item->childCount(); ++i) {
                    QTreeWidgetItem* child = item->child(i);

                    out << "<h2>" << child->text(0) << "</h2>\n";

                    switch (i) {
                    case 0: loadRegionalSettings(); break;
                    case 1: loadEnvironmentVariables(); break;
                    case 2: loadControlPanelInfo(); break;
                    case 3: loadTrashInfo(); break;
                    case 4: loadSystemFilesInfo(); break;
                    case 5: loadSystemFolders(); break;
                    case 6: loadEventLogs(); break;
                    }
                    writeTableToHtml();
                }
                ui.tableWidget->setRowCount(0);
                ui.stackedWidget->setCurrentIndex(1);
                loadRootMenu();
            }
            else {
                // ЧАСТКОВЫЙ ЗВІТ (тільки вибрана вкладка)
                int index = item->parent()->indexOfChild(item);
                switch (index) {
                case 0: loadRegionalSettings(); break;
                case 1: loadEnvironmentVariables(); break;
                case 2: loadControlPanelInfo(); break;
                case 3: loadTrashInfo(); break;
                case 4: loadSystemFilesInfo(); break;
                case 5: loadSystemFolders(); break;
                case 6: loadEventLogs(); break;
                }
                writeTableToHtml();
            }

            // Закриваємо теги HTML
            out << "</body>\n</html>\n";

            file.close();

            QMessageBox::information(this,
                isEnglish ? "Success" : QString::fromUtf8("Успіх"),
                isEnglish ? "HTML Report saved!" : QString::fromUtf8("HTML звіт успішно збережено!"));
        }
    }
    }
else if (selectedAction == actionMhtml) {
    // 1. Відкриваємо вікно збереження з форматами .mht або .mhtml
    QString fileName = QFileDialog::getSaveFileName(this,
        isEnglish ? "Save MHTML Report" : QString::fromUtf8("Зберегти MHTML звіт"),
        "", "MHTML Web Archive (*.mht *.mhtml);;All Files (*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);

            // 2. Пишемо службові заголовки формату MHTML (MIME)
            QString boundary = "----=_NextPart_System_Config_Report";
            out << "MIME-Version: 1.0\n";
            out << "Content-Type: multipart/related; boundary=\"" << boundary << "\"\n\n";

            out << "--" << boundary << "\n";
            out << "Content-Type: text/html; charset=\"utf-8\"\n";
            out << "Content-Transfer-Encoding: 8bit\n\n";

            // 3. Звідси починається звичайний HTML-код 
            out << "<!DOCTYPE html>\n<html>\n<head>\n";
            out << "<meta charset='UTF-8'>\n";
            out << "<title>" << (isEnglish ? "System Report" : QString::fromUtf8("Системний звіт")) << "</title>\n";
            out << "<style>\n";
            out << "body { font-family: 'Segoe UI', Tahoma, Arial, sans-serif; background-color: #f4f6f9; color: #333; margin: 20px; }\n";
            out << "h1 { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 10px; }\n";
            out << "h2 { color: #2980b9; margin-top: 30px; }\n";
            out << ".report-header { background-color: #fff; padding: 15px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-bottom: 20px; }\n";
            out << "table { width: 100%; border-collapse: collapse; margin-top: 10px; background-color: #fff; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }\n";
            out << "th { background-color: #34495e; color: #fff; padding: 12px; text-align: left; border: 1px solid #ddd; }\n";
            out << "td { padding: 10px 12px; border: 1px solid #ddd; }\n";
            out << "tr:nth-child(even) { background-color: #f9f9f9; }\n";
            out << "tr:hover { background-color: #f1f2f6; }\n";
            out << "</style>\n";
            out << "</head>\n<body>\n";

            out << "<div class='report-header'>\n";
            out << "<h1>" << (isEnglish ? "System Configuration Report" : QString::fromUtf8("Звіт конфігурації системи")) << "</h1>\n";
            out << "<p><b>" << (isEnglish ? "Target:" : QString::fromUtf8("Ціль:")) << "</b> " << item->text(0) << "</p>\n";
            out << "<p><b>" << (isEnglish ? "Date generated:" : QString::fromUtf8("Дата створення:")) << "</b> "
                << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "</p>\n";
            out << "</div>\n";

            // Функція для друку таблиці
            auto writeTableToHtml = [&]() {
                int rows = ui.tableWidget->rowCount();
                int cols = ui.tableWidget->columnCount();
                if (rows == 0) return;

                out << "<table>\n<tr>\n";
                for (int c = 0; c < cols; ++c) {
                    if (!ui.tableWidget->isColumnHidden(c)) {
                        QString hText = ui.tableWidget->horizontalHeaderItem(c) ? ui.tableWidget->horizontalHeaderItem(c)->text() : "";
                        out << "<th>" << hText.toHtmlEscaped() << "</th>\n";
                    }
                }
                out << "</tr>\n";
                for (int r = 0; r < rows; ++r) {
                    out << "<tr>\n";
                    for (int c = 0; c < cols; ++c) {
                        if (!ui.tableWidget->isColumnHidden(c)) {
                            QTableWidgetItem* cell = ui.tableWidget->item(r, c);
                            QString text = cell ? cell->text() : "";
                            out << "<td>" << text.toHtmlEscaped() << "</td>\n";
                        }
                    }
                    out << "</tr>\n";
                }
                out << "</table>\n";
                };

            // Генерація даних
            if (item->parent() == nullptr) {
                for (int i = 0; i < item->childCount(); ++i) {
                    QTreeWidgetItem* child = item->child(i);
                    out << "<h2>" << child->text(0) << "</h2>\n";
                    switch (i) {
                    case 0: loadRegionalSettings(); break;
                    case 1: loadEnvironmentVariables(); break;
                    case 2: loadControlPanelInfo(); break;
                    case 3: loadTrashInfo(); break;
                    case 4: loadSystemFilesInfo(); break;
                    case 5: loadSystemFolders(); break;
                    case 6: loadEventLogs(); break;
                    }
                    writeTableToHtml();
                }
                ui.tableWidget->setRowCount(0);
                ui.stackedWidget->setCurrentIndex(1);
                loadRootMenu();
            }
            else {
                int index = item->parent()->indexOfChild(item);
                switch (index) {
                case 0: loadRegionalSettings(); break;
                case 1: loadEnvironmentVariables(); break;
                case 2: loadControlPanelInfo(); break;
                case 3: loadTrashInfo(); break;
                case 4: loadSystemFilesInfo(); break;
                case 5: loadSystemFolders(); break;
                case 6: loadEventLogs(); break;
                }
                writeTableToHtml();
            }

            out << "</body>\n</html>\n\n";

            // 4. Закриваємо MHTML архів
            out << "--" << boundary << "--\n";

            file.close();

            QMessageBox::information(this,
                isEnglish ? "Success" : QString::fromUtf8("Успіх"),
                isEnglish ? "MHTML Report saved!" : QString::fromUtf8("MHTML звіт успішно збережено!"));
        }
    }
    }
        });

    // ==========================================================================
        // ЛОГІКА ДЕРЕВА
        // ==========================================================================
    connect(ui.treeWidget, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int column) {
        if (!item) return;

        QTreeWidgetItem* parentItem = item->parent();

        if (parentItem != nullptr) {
            ui.stackedWidget->setCurrentIndex(0);  // Показуємо сторінку 0 (ТАБЛИЦЯ)

            ui.tableWidget->setRowCount(0);
            int index = parentItem->indexOfChild(item);
            switch (index) {
            case 0: loadRegionalSettings(); break;
            case 1: loadEnvironmentVariables(); break;
            case 2: loadControlPanelInfo(); break;
            case 3: loadTrashInfo(); break;
            case 4: loadSystemFilesInfo(); break;
            case 5: loadSystemFolders(); break;
            case 6: loadEventLogs(); break;
            }
        }
        else {
            ui.stackedWidget->setCurrentIndex(1); // Показуємо сторінку 1 (ЯРЛИКИ)
            loadRootMenu();
        }
        });
    // ==========================================================================
    // ЛОГІКА КЛІКУ ПО ЯРЛИКУ (Щоб переходити в розділ)
    // ==========================================================================
    connect(ui.listWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        int row = ui.listWidget->row(item); // Отримуємо номер ярлика по якому клікнули
        QTreeWidgetItem* root = ui.treeWidget->topLevelItem(0); // Знаходимо "Конфігурацію"

        if (root) {
            QTreeWidgetItem* child = root->child(row); // Знаходимо відповідний підпункт у дереві
            if (child) {
                ui.treeWidget->setCurrentItem(child); // Робимо його активним у дереві зліва
                emit ui.treeWidget->itemClicked(child, 0); // Програмно "натискаємо" на нього
            }
        }
        });

    // ==========================================================================
      // ЛОГІКА ТАБЛИЦІ (Всі розділи)
      // ==========================================================================
    connect(ui.tableWidget, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        QTreeWidgetItem* currentItem = ui.treeWidget->currentItem();
        if (!currentItem || currentItem->parent() == nullptr) return;

        int index = currentItem->parent()->indexOfChild(currentItem);

        if (index == 0) { // 0. РЕГІОНАЛЬНІ УСТАНОВКИ (Відкриваємо вікно Windows)
            // Викликаємо стандартний аплет Windows 
            QProcess::startDetached("control.exe", { "intl.cpl" });
        }
        else if (index == 2) { // 1. Панель керування
            QTableWidgetItem* clsidItem = ui.tableWidget->item(row, 2);
            if (clsidItem) {
                QString clsid = clsidItem->text();
                QProcess::startDetached("explorer.exe", { "shell:::" + clsid });
            }
        }
        else if (index == 3) { // 2. Кошик
            HRESULT result = SHEmptyRecycleBinW(reinterpret_cast<HWND>(this->winId()), NULL, 0);
            if (result == S_OK) loadTrashInfo();
        }
        else if (index == 4) { // 3. Системні файли
            QTableWidgetItem* nameItem = ui.tableWidget->item(row, 0);
            QTableWidgetItem* pathItem = ui.tableWidget->item(row, 2);
            if (nameItem && pathItem) {
                QString fullFilePath = pathItem->text() + "\\" + nameItem->text();
                if (QFile::exists(fullFilePath)) {
                    QProcess::startDetached("notepad.exe", { fullFilePath });
                }
            }
        }
        else if (index == 5) { // 4. Системні папки
            QTableWidgetItem* pathItem = ui.tableWidget->item(row, 1);
            if (pathItem) QProcess::startDetached("explorer.exe", { pathItem->text() });
        }
        else if (index == 6) { // 5. ПРОТОКОЛИ ПОДІЙ (Наше власне вікно властивостей)
            QString logName = ui.tableWidget->item(row, 0)->text();
            QString type = ui.tableWidget->item(row, 1)->text();
            QString category = ui.tableWidget->item(row, 2)->text();
            QString dateTime = ui.tableWidget->item(row, 3)->text();
            QString user = ui.tableWidget->item(row, 4)->text();
            QString source = ui.tableWidget->item(row, 5)->text();
            QString description = ui.tableWidget->item(row, 6)->text();

            QStringList dtParts = dateTime.split(" ");
            QString date = dtParts.value(0);
            QString time = dtParts.value(1);

            QDialog dialog(this);
            dialog.setWindowTitle(isEnglish ? "Event Properties" : QString::fromUtf8("Властивості події"));
            dialog.resize(600, 450);

            QGridLayout* layout = new QGridLayout(&dialog);

            layout->addWidget(new QLabel(isEnglish ? "<b>Date:</b>" : QString::fromUtf8("<b>Дата:</b>")), 0, 0);
            layout->addWidget(new QLabel(date), 0, 1);
            layout->addWidget(new QLabel(isEnglish ? "<b>Time:</b>" : QString::fromUtf8("<b>Час:</b>")), 1, 0);
            layout->addWidget(new QLabel(time), 1, 1);
            layout->addWidget(new QLabel(isEnglish ? "<b>Type:</b>" : QString::fromUtf8("<b>Тип:</b>")), 2, 0);
            layout->addWidget(new QLabel(type), 2, 1);
            layout->addWidget(new QLabel(isEnglish ? "<b>User:</b>" : QString::fromUtf8("<b>Користувач:</b>")), 3, 0);
            layout->addWidget(new QLabel(user), 3, 1);

            layout->addWidget(new QLabel(isEnglish ? "<b>Log:</b>" : QString::fromUtf8("<b>Протокол:</b>")), 0, 2);
            layout->addWidget(new QLabel(logName), 0, 3);
            layout->addWidget(new QLabel(isEnglish ? "<b>Category:</b>" : QString::fromUtf8("<b>Категорія:</b>")), 1, 2);
            layout->addWidget(new QLabel(category), 1, 3);
            layout->addWidget(new QLabel(isEnglish ? "<b>Source:</b>" : QString::fromUtf8("<b>Джерело:</b>")), 2, 2);
            layout->addWidget(new QLabel(source), 2, 3);

            QLabel* descLabel = new QLabel(isEnglish ? "<b>Description:</b>" : QString::fromUtf8("<b>Опис:</b>"));
            layout->addWidget(descLabel, 4, 0, 1, 4);

            QTextEdit* textEdit = new QTextEdit();
            textEdit->setReadOnly(true);
            textEdit->setText(description);
            layout->addWidget(textEdit, 5, 0, 1, 4);

            dialog.exec();
        }
        });
}

SystemConfigurationGUI::~SystemConfigurationGUI() {}

void SystemConfigurationGUI::loadRegionalSettings() {
    ui.tableWidget->setSortingEnabled(false);
    ui.tableWidget->setRowCount(0);
    ui.tableWidget->setColumnCount(2);
    // Оновлюємо колонки залежно від мови
    if (isEnglish) {
        ui.tableWidget->setHorizontalHeaderLabels({ "Property", "Value" });
    }
    else {
        ui.tableWidget->setHorizontalHeaderLabels({ QString::fromUtf8("Властивість"), QString::fromUtf8("Значення") });
    }

    auto addRow = [this](QString prop, QString val) {
        int r = ui.tableWidget->rowCount();
        ui.tableWidget->insertRow(r);
        ui.tableWidget->setItem(r, 0, new QTableWidgetItem(prop));
        ui.tableWidget->setItem(r, 1, new QTableWidgetItem(val));
        };

    auto addCategory = [this](QString categoryName) {
        int r = ui.tableWidget->rowCount();
        ui.tableWidget->insertRow(r);
        QTableWidgetItem* item = new QTableWidgetItem(categoryName);
        QFont font = item->font(); font.setBold(true); item->setFont(font);
        item->setBackground(QBrush(QColor(230, 230, 230)));
        ui.tableWidget->setItem(r, 0, item);
        ui.tableWidget->setItem(r, 1, new QTableWidgetItem(""));
        };

    auto getLoc = [](LCTYPE type) -> QString {
        wchar_t buf[256] = { 0 };
        if (GetLocaleInfoW(LOCALE_USER_DEFAULT, type, buf, 256)) return QString::fromWCharArray(buf);
        return "";
        };

    // ================== ЧАСОВА ЗОНА ==================
    addCategory(isEnglish ? "Time Zone" : QString::fromUtf8("Часова зона"));
    DYNAMIC_TIME_ZONE_INFORMATION dtz;
    DWORD result = GetDynamicTimeZoneInformation(&dtz);

    QString currentName = (result == TIME_ZONE_ID_DAYLIGHT) ? QString::fromWCharArray(dtz.DaylightName) : QString::fromWCharArray(dtz.StandardName);
    addRow(isEnglish ? "Current time zone" : QString::fromUtf8("Поточна часова зона"), currentName);

    HKEY hKey;
    std::wstring subKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" + std::wstring(dtz.TimeZoneKeyName);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t displayBuffer[256] = { 0 };
        DWORD bufferSize = sizeof(displayBuffer);
        if (RegQueryValueExW(hKey, L"Display", NULL, NULL, (LPBYTE)displayBuffer, &bufferSize) == ERROR_SUCCESS) {
            addRow(isEnglish ? "Current time zone description" : QString::fromUtf8("Опис поточної часової зони"), QString::fromWCharArray(displayBuffer));
        }
        RegCloseKey(hKey);
    }

    auto formatTransitionDate = [this](const SYSTEMTIME& st) -> QString {
        if (st.wMonth == 0) return isEnglish ? "None" : QString::fromUtf8("Ніколи");
        QString days[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
        QString months[] = { "", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
        QString weekNum;
        if (st.wDay == 1) weekNum = "First";
        else if (st.wDay == 2) weekNum = "Second";
        else if (st.wDay == 3) weekNum = "Third";
        else if (st.wDay == 4) weekNum = "Fourth";
        else if (st.wDay == 5) weekNum = "Last";

        return QString("%1 %2 of %3 %4:%5:%6")
            .arg(weekNum).arg(days[st.wDayOfWeek]).arg(months[st.wMonth])
            .arg(st.wHour).arg(st.wMinute, 2, 10, QChar('0')).arg(st.wSecond, 2, 10, QChar('0'));
        };

    addRow(isEnglish ? "Transition to Standard Time" : QString::fromUtf8("Перехід на стандартний час"), formatTransitionDate(dtz.StandardDate));
    addRow(isEnglish ? "Transition to Daylight Time" : QString::fromUtf8("Перехід на літній час"), formatTransitionDate(dtz.DaylightDate));

    // ================== МОВА ==================
    addCategory(isEnglish ? "Language" : QString::fromUtf8("Мова"));
    addRow(isEnglish ? "Language (system)" : QString::fromUtf8("Мова (сист.)"), getLoc(LOCALE_SNATIVELANGNAME));
    addRow(isEnglish ? "Language (English)" : QString::fromUtf8("Мова (англ.)"), getLoc(LOCALE_SENGLANGUAGE));
    addRow(isEnglish ? "Language (ISO 639)" : QString::fromUtf8("Мова (ISO 639)"), getLoc(LOCALE_SISO639LANGNAME));

    // ================== КРАЇНА/РЕГІОН ==================
    addCategory(isEnglish ? "Country/Region" : QString::fromUtf8("Країна/Регіон"));
    addRow(isEnglish ? "Country (system)" : QString::fromUtf8("Країна (сист.)"), getLoc(LOCALE_SNATIVECOUNTRYNAME));
    addRow(isEnglish ? "Country (English)" : QString::fromUtf8("Країна (англ.)"), getLoc(LOCALE_SENGCOUNTRY));
    addRow(isEnglish ? "Country (ISO 3166)" : QString::fromUtf8("Країна (ISO 3166)"), getLoc(LOCALE_SISO3166CTRYNAME));
    addRow(isEnglish ? "Country Code" : QString::fromUtf8("Код країни"), getLoc(LOCALE_ICOUNTRY));

    // ================== ГРОШОВА ОДИНИЦЯ ==================
    addCategory(isEnglish ? "Currency" : QString::fromUtf8("Грошова одиниця"));
    QString nativeCurr = getLoc(0x00001008);
    if (nativeCurr.isEmpty()) nativeCurr = isEnglish ? "hryvnia" : QString::fromUtf8("гривня");

    addRow(isEnglish ? "Currency (system)" : QString::fromUtf8("Грошова одиниця (сист.)"), nativeCurr);
    addRow(isEnglish ? "Currency (English)" : QString::fromUtf8("Грошова одиниця (англ.)"), getLoc(0x00001007));

    QString symNat = getLoc(LOCALE_SCURRENCY);
    addRow(isEnglish ? "Currency Symbol (system)" : QString::fromUtf8("Символ грошової одиниці (сист.)"), symNat);
    addRow(isEnglish ? "Currency Symbol (ISO)" : QString::fromUtf8("Символ грошової одиниці (ISO)"), getLoc(LOCALE_SINTLSYMBOL));

    QString posFormat = isEnglish ? "Positive Currency Format" : QString::fromUtf8("Формат грошових сум");
    QString negFormat = isEnglish ? "Negative Currency Format" : QString::fromUtf8("Формат негативних грошових сум");
    addRow(posFormat, "123 456 789,00 " + symNat);
    addRow(negFormat, "-123 456 789,00 " + symNat);

    // ================== ФОРМАТУВАННЯ ==================
    addCategory(isEnglish ? "Formatting" : QString::fromUtf8("Форматування"));
    addRow(isEnglish ? "Time Format" : QString::fromUtf8("Формат часу"), getLoc(LOCALE_STIMEFORMAT));
    addRow(isEnglish ? "Short Date Format" : QString::fromUtf8("Короткий формат дати"), getLoc(LOCALE_SSHORTDATE));
    addRow(isEnglish ? "Long Date Format" : QString::fromUtf8("Повний формат дати"), getLoc(LOCALE_SLONGDATE));

    QString sep = getLoc(LOCALE_STHOUSAND);
    QString dec = getLoc(LOCALE_SDECIMAL);
    QString numFmt = "123" + sep + "456" + sep + "789" + dec + "00";
    addRow(isEnglish ? "Number Format" : QString::fromUtf8("Формат виводу чисел"), numFmt);
    addRow(isEnglish ? "Negative Number Format" : QString::fromUtf8("Формат виводу негативних..."), "-" + numFmt);

    QString lSep = getLoc(LOCALE_SLIST);
    addRow(isEnglish ? "List Format" : QString::fromUtf8("Формат списку"), "first" + lSep + " second" + lSep + " third");
    addRow(isEnglish ? "Native Digits" : QString::fromUtf8("Набір цифр"), getLoc(LOCALE_SNATIVEDIGITS));

    // ================== ДНІ ТИЖНЯ ==================
    addCategory(isEnglish ? "Weekdays" : QString::fromUtf8("Дні тижня"));

    QString daysUk[] = { "понеділка", "вівторка", "середи", "четверга", "п'ятниці", "суботи", "неділі" };
    QString daysEn[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };

    for (int i = 0; i < 7; ++i) {
        QString full = getLoc(LOCALE_SDAYNAME1 + i);
        QString shortName = getLoc(LOCALE_SABBREVDAYNAME1 + i);
        QString propName = isEnglish ? ("Name for " + daysEn[i]) : (QString::fromUtf8("Ім'я для ") + daysUk[i]);
        addRow(propName, full + " / " + shortName);
    }

    // ================== МІСЯЦІ ==================
    addCategory(isEnglish ? "Months" : QString::fromUtf8("Місяці"));

    QString monthsUk[] = { "січня", "лютого", "березня", "квітня", "травня", "червня", "липня", "серпня", "вересня", "жовтня", "листопада", "грудня" };
    QString monthsEn[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

    for (int i = 0; i < 12; ++i) {
        QString full = getLoc(LOCALE_SMONTHNAME1 + i);
        QString shortName = getLoc(LOCALE_SABBREVMONTHNAME1 + i);
        QString propName = isEnglish ? ("Name for " + monthsEn[i]) : (QString::fromUtf8("Ім'я для ") + monthsUk[i]);
        addRow(propName, full + " / " + shortName);
    }

    // ================== РІЗНЕ ==================
    addCategory(isEnglish ? "Miscellaneous" : QString::fromUtf8("Різне"));

    DWORD calType;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ICALENDARTYPE | LOCALE_RETURN_NUMBER, (LPWSTR)&calType, sizeof(calType) / sizeof(wchar_t));
    QString calString = (calType == 1) ? "Gregorian (localized)" : "Other";
    addRow(isEnglish ? "Calendar Type" : QString::fromUtf8("Тип календаря"), calString);

    DWORD paperSize;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IPAPERSIZE | LOCALE_RETURN_NUMBER, (LPWSTR)&paperSize, sizeof(paperSize) / sizeof(wchar_t));
    addRow(isEnglish ? "Default Paper Size" : QString::fromUtf8("Розмір паперу за замовчуванням"), (paperSize == 9) ? "A4" : "Letter");

    DWORD measure;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IMEASURE | LOCALE_RETURN_NUMBER, (LPWSTR)&measure, sizeof(measure) / sizeof(wchar_t));
    QString measureStr = (measure == 0) ? (isEnglish ? "Metric" : QString::fromUtf8("Метрична")) : (isEnglish ? "US (Imperial)" : QString::fromUtf8("Американська (Imperial)"));
    addRow(isEnglish ? "Measurement System" : QString::fromUtf8("Система числення"), measureStr);

    // ================== МОВНІ ПАКЕТИ (LCID) ==================
    addCategory(isEnglish ? "Installed System Language Packs" : QString::fromUtf8("Інстальовані мовні пакети системи"));
    std::vector<LanguagePackage> languages;
    g_installedLangs = &languages;
    EnumSystemLocalesW((LOCALE_ENUMPROCW)EnumLocalesProcQt, LCID_INSTALLED);
    g_installedLangs = nullptr;

    for (const auto& pkg : languages) {
        QString title = pkg.lcidStr;
        if (pkg.isActive) {
            title += isEnglish ? " (Active)" : QString::fromUtf8(" (Активний)");
        }
        addRow(title, pkg.name);
    }
}

// Допоміжна функція для читання змінних з реєстру 
void GetVarsFromRegistryQt(HKEY hRoot, const wchar_t* subKey, std::vector<std::pair<QString, QString>>& vars) {
    HKEY hKey;
    if (RegOpenKeyExW(hRoot, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t valueName[16383];
        byte valueData[32767];
        DWORD nameLen, dataLen, type;

        for (DWORD i = 0; ; i++) {
            nameLen = 16383;
            dataLen = 32767;
            if (RegEnumValueW(hKey, i, valueName, &nameLen, NULL, &type, valueData, &dataLen) != ERROR_SUCCESS)
                break;

            QString name = QString::fromWCharArray(valueName);
            QString value;

            if (type == REG_SZ || type == REG_EXPAND_SZ) {
                value = QString::fromWCharArray((wchar_t*)valueData);
            }

            bool exists = false;
            for (const auto& v : vars) {
                if (v.first == name) { exists = true; break; }
            }
            if (!exists && !name.isEmpty()) vars.push_back({ name, value });
        }
        RegCloseKey(hKey);
    }
}

// ==============================================================================
void SystemConfigurationGUI::loadEnvironmentVariables() {
    ui.tableWidget->setSortingEnabled(false);
    ui.tableWidget->setRowCount(0);
    ui.tableWidget->setColumnCount(2);

    // Оновлюємо колонки залежно від мови
    if (isEnglish) {
        ui.tableWidget->setHorizontalHeaderLabels({ "Variable", "Value" });
    }
    else {
        ui.tableWidget->setHorizontalHeaderLabels({ QString::fromUtf8("Змінна"), QString::fromUtf8("Значення") });
    }

    std::vector<std::pair<QString, QString>> vars;

    LPWCH lpszVariable = GetEnvironmentStringsW();
    if (lpszVariable != NULL) {
        LPWCH lpszCurrent = lpszVariable;
        while (*lpszCurrent != L'\0') {
            std::wstring entry(lpszCurrent);
            size_t pos = entry.find(L'=');
            if (pos != std::wstring::npos && pos > 0) {
                QString name = QString::fromStdWString(entry.substr(0, pos));
                QString value = QString::fromStdWString(entry.substr(pos + 1));
                if (name[0] != QChar('=')) {
                    vars.push_back({ name, value });
                }
            }
            lpszCurrent += entry.length() + 1;
        }
        FreeEnvironmentStringsW(lpszVariable);
    }

    GetVarsFromRegistryQt(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Session Manager\\Environment", vars);
    GetVarsFromRegistryQt(HKEY_CURRENT_USER, L"Environment", vars);

    std::sort(vars.begin(), vars.end(), [](const std::pair<QString, QString>& a, const std::pair<QString, QString>& b) {
        return a.first < b.first;
        });

    for (const auto& var : vars) {
        int r = ui.tableWidget->rowCount();
        ui.tableWidget->insertRow(r);

        QTableWidgetItem* nameItem = new QTableWidgetItem(var.first);
        QTableWidgetItem* valueItem = new QTableWidgetItem(var.second);

        valueItem->setToolTip(var.second);

        ui.tableWidget->setItem(r, 0, nameItem);
        ui.tableWidget->setItem(r, 1, valueItem);
    }

    ui.tableWidget->setWordWrap(false);
    ui.tableWidget->resizeColumnToContents(0);
    ui.tableWidget->setSortingEnabled(true);
}


// Допоміжна функція для отримання локалізованих рядків Панелі керування
std::wstring GetLocalizedStringQt(const wchar_t* source) {
    if (!source || source[0] == L'\0') return L"";
    if (source[0] != L'@') return std::wstring(source);

    wchar_t outBuffer[512];
    if (SHLoadIndirectString(source, outBuffer, 512, NULL) == S_OK) {
        return std::wstring(outBuffer);
    }
    return std::wstring(source);
}

void SystemConfigurationGUI::loadControlPanelInfo() {
    ui.tableWidget->setSortingEnabled(false);
    ui.tableWidget->setRowCount(0);
    // Робимо 3 колонки. Третя буде прихована для користувача, там лежатиме CLSID для запуску
    ui.tableWidget->setColumnCount(3);
    ui.tableWidget->setColumnHidden(2, true);

    if (isEnglish) {
        ui.tableWidget->setHorizontalHeaderLabels({ "Control Panel Item", "Description", "CLSID" });
    }
    else {
        ui.tableWidget->setHorizontalHeaderLabels({ QString::fromUtf8("Елемент панелі керування"), QString::fromUtf8("Опис"), "CLSID" });
    }

    // ТУТ МАЄ БУТИ ПОВНА МАПА, ЯК У ТВОЄМУ КОНСОЛЬНОМУ КОДІ
    std::map<std::wstring, std::wstring> translate = {
        { L"Secure Startup", L"BitLocker Drive Encryption" },
        { L"ECS", L"Work Folders" },
        { L"AutoPlay", L"Автовідтворення" },
        { L"History Vault", L"Банк файлів" },
        { L"Windows Defender Firewall", L"Брандмауер для Захисника Windows" },
        { L"Troubleshooting", L"Виправлення неполадок" },
        { L"System Recovery", L"Відновлення" },
        { L"Internet Options", L"Властивості браузера" },
        { L"Date and Time Control Panel", L"Дата й час" },
        { L"Credential Manager", L"Диспетчер облікових даних" },
        { L"Device Manager", L"Диспетчер пристроїв" },
        { L"Power Options", L"Електроживлення" },
        { L"Sound Control Panel", L"Звук" },
        { L"Windows Tools", L"Інструменти Windows" },
        { L"Color Management", L"Керування кольором" },
        { L"Keyboard Control Panel", L"Клавіатура" },
        { L"Mouse Control Panel", L"Миша" },
        { L"User Accounts", L"Облікові записи користувачів" },
        { L"Security and Maintenance CPL", L"Обслуговування та безпека" },
        { L"Taskbar", L"Панель завдань і переходи" },
        { L"Indexing Options Control Panel", L"Параметри індексування" },
        { L"Folder Options", L"Параметри Файлового провідника" },
        { L"Workspaces Center", L"Підключення до віддалених робочих столів" },
        { L"Device Center", L"Пристрої та принтери" },
        { L"Set User Defaults", L"Програми за замовчуванням" },
        { L"Programs and Features", L"Програми та засоби" },
        { L"Storage Spaces", L"Простори зберігання" },
        { L"Region and Language", L"Регіон" },
        { L"Windows 7 File Recovery", L"Резервне копіювання (Windows 7)" },
        { L"System", L"Система" },
        { L"Phone and Modem Control Panel", L"Телефон і модем" },
        { L"Ease of Access", L"Центр легкого доступу" },
        { L"Network and Sharing Center", L"Центр мережевих підключень" },
        { L"Mobility Center Control Panel", L"Центр підтримки портативних ПК" },
        { L"Sync Center Folder", L"Центр синхронізації" },
        { L"Microsoft Windows Font Folder", L"Шрифти" }
    };

    struct CPLItem { QString name; QString comment; QString clsid; };
    std::vector<CPLItem> items;

    HKEY hNamespaceKey;
    const wchar_t* nsPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace";

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, nsPath, 0, KEY_READ, &hNamespaceKey) == ERROR_SUCCESS) {
        wchar_t clsidName[MAX_PATH];
        DWORD nameSize = MAX_PATH;

        for (DWORD i = 0; RegEnumKeyExW(hNamespaceKey, i, clsidName, &nameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; i++) {
            nameSize = MAX_PATH;
            std::wstring clsidPath = L"Software\\Classes\\CLSID\\" + std::wstring(clsidName);
            HKEY hItemKey;

            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, clsidPath.c_str(), 0, KEY_READ, &hItemKey) == ERROR_SUCCESS) {
                wchar_t rawName[512] = { 0 }, rawComment[512] = { 0 };
                DWORD valSize = sizeof(rawName);

                if (RegGetValueW(hItemKey, NULL, L"LocalizedDisplayName", RRF_RT_REG_SZ, NULL, rawName, &valSize) != ERROR_SUCCESS) {
                    valSize = sizeof(rawName);
                    RegGetValueW(hItemKey, NULL, NULL, RRF_RT_REG_SZ, NULL, rawName, &valSize);
                }

                valSize = sizeof(rawComment);
                RegGetValueW(hItemKey, NULL, L"InfoTip", RRF_RT_REG_SZ, NULL, rawComment, &valSize);

                std::wstring processedName = GetLocalizedStringQt(rawName);
                QString displayItemName = QString::fromStdWString(processedName);

                // Застосовуємо логіку перекладу ТІЛЬКИ якщо вибрана українська
                if (!isEnglish) {
                    if (translate.count(processedName)) {
                        displayItemName = QString::fromStdWString(translate[processedName]);
                    }
                }

                if (!displayItemName.isEmpty()) {
                    items.push_back({
                        displayItemName,
                        QString::fromStdWString(GetLocalizedStringQt(rawComment)),
                        QString::fromWCharArray(clsidName) 
                        });
                }
                RegCloseKey(hItemKey);
            }
        }
        RegCloseKey(hNamespaceKey);
    }

    std::sort(items.begin(), items.end(), [](const CPLItem& a, const CPLItem& b) { return a.name < b.name; });

    for (const auto& item : items) {
        int r = ui.tableWidget->rowCount();
        ui.tableWidget->insertRow(r);
        ui.tableWidget->setItem(r, 0, new QTableWidgetItem(item.name));
        ui.tableWidget->setItem(r, 1, new QTableWidgetItem(item.comment));
        ui.tableWidget->setItem(r, 2, new QTableWidgetItem(item.clsid)); // Записуємо CLSID у приховану колонку
    }
    ui.tableWidget->resizeColumnToContents(0);
    ui.tableWidget->setSortingEnabled(true);
}

void SystemConfigurationGUI::loadTrashInfo() {
    ui.tableWidget->setSortingEnabled(false);
    ui.tableWidget->setRowCount(0);
    ui.tableWidget->setColumnCount(5);

    if (isEnglish) {
        ui.tableWidget->setHorizontalHeaderLabels({ "Drive", "Status", "Items", "Used Space", "% of Drive" });
    }
    else {
        ui.tableWidget->setHorizontalHeaderLabels({ QString::fromUtf8("Диск"), QString::fromUtf8("Статус"), QString::fromUtf8("Елементи"), QString::fromUtf8("Зайнятий простір"), QString::fromUtf8("Відсоток від диска") });
    }

    wchar_t szDrives[256];
    if (GetLogicalDriveStringsW(256, szDrives) == 0) return;

    wchar_t* pDrive = szDrives;
    while (*pDrive) {
        if (GetDriveTypeW(pDrive) == DRIVE_FIXED) {
            SHQUERYRBINFO rbInfo = { sizeof(rbInfo) };
            if (SHQueryRecycleBinW(pDrive, &rbInfo) == S_OK) {
                QString driveName = QString::fromWCharArray(pDrive).left(2);
                QString status = (rbInfo.i64NumItems > 0) ? (isEnglish ? "Full" : QString::fromUtf8("Повний")) : (isEnglish ? "Empty" : QString::fromUtf8("Порожній"));

                double sizeMB = (double)rbInfo.i64Size / (1024.0 * 1024.0);
                QString spaceUsed = QString::number(sizeMB, 'f', 2) + " MB";

                QString percentUsed = "0.00 %";
                ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
                if (GetDiskFreeSpaceExW(pDrive, &freeBytes, &totalBytes, &totalFreeBytes) && totalBytes.QuadPart > 0) {
                    percentUsed = QString::number((double)rbInfo.i64Size / (double)totalBytes.QuadPart * 100.0, 'f', 4) + " %";
                }

                int r = ui.tableWidget->rowCount();
                ui.tableWidget->insertRow(r);
                ui.tableWidget->setItem(r, 0, new QTableWidgetItem(driveName));
                ui.tableWidget->setItem(r, 1, new QTableWidgetItem(status));
                ui.tableWidget->setItem(r, 2, new QTableWidgetItem(QString::number(rbInfo.i64NumItems)));
                ui.tableWidget->setItem(r, 3, new QTableWidgetItem(spaceUsed));
                ui.tableWidget->setItem(r, 4, new QTableWidgetItem(percentUsed));
            }
        }
        pDrive += wcslen(pDrive) + 1;
    }
    ui.tableWidget->resizeColumnsToContents();
    ui.tableWidget->setSortingEnabled(true);
}
////////////////////////44444/////////////
void SystemConfigurationGUI::loadSystemFilesInfo() {
    ui.tableWidget->setSortingEnabled(false);
    ui.tableWidget->setRowCount(0);
    ui.tableWidget->setColumnCount(3);

    // Переклад заголовків
    if (isEnglish) {
        ui.tableWidget->setHorizontalHeaderLabels({ "File Name", "Size", "Path" });
    }
    else {
        ui.tableWidget->setHorizontalHeaderLabels({ QString::fromUtf8("Ім'я файлу"), QString::fromUtf8("Розмір"), QString::fromUtf8("Шлях") });
    }

    auto getFileSize = [](const std::wstring& fullPath) -> long long {
        WIN32_FILE_ATTRIBUTE_DATA fad;
        if (GetFileAttributesExW(fullPath.c_str(), GetFileExInfoStandard, &fad)) {
            LARGE_INTEGER size;
            size.HighPart = fad.nFileSizeHigh;
            size.LowPart = fad.nFileSizeLow;
            return size.QuadPart;
        }
        return 0;
        };

    auto formatSize = [this](long long bytes) -> QString {
        if (bytes > 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " MB";
        if (bytes > 1024) return QString::number(bytes / 1024.0, 'f', 2) + " KB";
        return QString::number(bytes) + (isEnglish ? " bytes" : QString::fromUtf8(" байт"));
        };

    wchar_t winDir[MAX_PATH];
    GetWindowsDirectoryW(winDir, MAX_PATH);
    std::wstring windowsPath(winDir);

    std::wstring searchPattern = windowsPath + L"\\*.ini";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                LARGE_INTEGER size;
                size.HighPart = findData.nFileSizeHigh;
                size.LowPart = findData.nFileSizeLow;

                int r = ui.tableWidget->rowCount();
                ui.tableWidget->insertRow(r);
                ui.tableWidget->setItem(r, 0, new QTableWidgetItem(QString::fromWCharArray(findData.cFileName)));
                ui.tableWidget->setItem(r, 1, new QTableWidgetItem(formatSize(size.QuadPart)));
                ui.tableWidget->setItem(r, 2, new QTableWidgetItem(QString::fromStdWString(windowsPath)));
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }

    std::wstring etcPath = windowsPath + L"\\system32\\drivers\\etc";
    std::vector<std::wstring> specialFiles = { L"hosts", L"lmhosts.sam" };

    for (const auto& sFile : specialFiles) {
        std::wstring fullPath = etcPath + L"\\" + sFile;
        long long size = getFileSize(fullPath);
        if (size > 0) {
            int r = ui.tableWidget->rowCount();
            ui.tableWidget->insertRow(r);
            ui.tableWidget->setItem(r, 0, new QTableWidgetItem(QString::fromStdWString(sFile)));
            ui.tableWidget->setItem(r, 1, new QTableWidgetItem(formatSize(size)));
            ui.tableWidget->setItem(r, 2, new QTableWidgetItem(QString::fromStdWString(etcPath)));
        }
    }
    ui.tableWidget->setWordWrap(false);
    ui.tableWidget->resizeColumnToContents(0);
    ui.tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui.tableWidget->setSortingEnabled(true);
}

void SystemConfigurationGUI::loadSystemFolders() {
    ui.tableWidget->setSortingEnabled(false);
    ui.tableWidget->setRowCount(0);
    ui.tableWidget->setColumnCount(2);

    if (isEnglish) {
        ui.tableWidget->setHorizontalHeaderLabels({ "Folder", "Path" });
    }
    else {
        ui.tableWidget->setHorizontalHeaderLabels({ QString::fromUtf8("Папка"), QString::fromUtf8("Шлях") });
    }

    struct FolderTarget { QString label; int csidl; };
    std::vector<FolderTarget> targets = {
        { "Administrative Tools", CSIDL_ADMINTOOLS }, { "AppData", CSIDL_APPDATA },
        { "Cache", CSIDL_INTERNET_CACHE }, { "CD Burning", CSIDL_CDBURN_AREA },
        { "Common Administrative Tools", CSIDL_COMMON_ADMINTOOLS }, { "Common AppData", CSIDL_COMMON_APPDATA },
        { "Common Desktop", CSIDL_COMMON_DESKTOPDIRECTORY }, { "Common Documents", CSIDL_COMMON_DOCUMENTS },
        { "Common Favorites", CSIDL_COMMON_FAVORITES }, { "Common Files (x86)", CSIDL_PROGRAM_FILES_COMMONX86 },
        { "Common Files", CSIDL_PROGRAM_FILES_COMMON }, { "Common Music", CSIDL_COMMON_MUSIC },
        { "Common Pictures", CSIDL_COMMON_PICTURES }, { "Common Programs", CSIDL_COMMON_PROGRAMS },
        { "Common Start Menu", CSIDL_COMMON_STARTMENU }, { "Common Startup", CSIDL_COMMON_STARTUP },
        { "Common Templates", CSIDL_COMMON_TEMPLATES }, { "Common Video", CSIDL_COMMON_VIDEO },
        { "Cookies", CSIDL_COOKIES }, { "Desktop", CSIDL_DESKTOP },
        { "Favorites", CSIDL_FAVORITES }, { "Fonts", CSIDL_FONTS },
        { "History", CSIDL_HISTORY }, { "Local AppData", CSIDL_LOCAL_APPDATA },
        { "My Documents", CSIDL_PERSONAL }, { "My Music", CSIDL_MYMUSIC },
        { "My Pictures", CSIDL_MYPICTURES }, { "My Video", CSIDL_MYVIDEO },
        { "NetHood", CSIDL_NETHOOD }, { "PrintHood", CSIDL_PRINTHOOD },
        { "Program Files (x86)", CSIDL_PROGRAM_FILESX86 }, { "Program Files", CSIDL_PROGRAM_FILES },
        { "Programs", CSIDL_PROGRAMS }, { "Recent", CSIDL_RECENT },
        { "Resources", CSIDL_RESOURCES }, { "SendTo", CSIDL_SENDTO },
        { "Start Menu", CSIDL_STARTMENU }, { "Startup", CSIDL_STARTUP },
        { "System (x86)", CSIDL_SYSTEMX86 }, { "System", CSIDL_SYSTEM },
        { "Templates", CSIDL_TEMPLATES }, { "Windows", CSIDL_WINDOWS }
    };

    std::vector<std::pair<QString, QString>> folders;
    for (const auto& target : targets) {
        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, target.csidl, NULL, 0, path))) {
            folders.push_back({ target.label, QString::fromWCharArray(path) });
        }
    }

    wchar_t profilePath[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERPROFILE", profilePath, MAX_PATH))
        folders.push_back({ "Profile", QString::fromWCharArray(profilePath) });

    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath)) {
        QString tPath = QString::fromWCharArray(tempPath);
        if (tPath.endsWith('\\')) tPath.chop(1);
        folders.push_back({ "Temp", tPath });
    }

    std::sort(folders.begin(), folders.end(), [](const std::pair<QString, QString>& a, const std::pair<QString, QString>& b) {
        return a.first < b.first;
        });

    for (const auto& f : folders) {
        int r = ui.tableWidget->rowCount();
        ui.tableWidget->insertRow(r);
        QTableWidgetItem* pathItem = new QTableWidgetItem(f.second);
        pathItem->setToolTip(f.second);
        ui.tableWidget->setItem(r, 0, new QTableWidgetItem(f.first));
        ui.tableWidget->setItem(r, 1, pathItem);
    }
    ui.tableWidget->resizeColumnToContents(0);
    ui.tableWidget->setSortingEnabled(true);
}


///////////////////////6666666666666666666///////////////////
// --- ДОПОМІЖНА ФУНКЦІЯ ДЛЯ ОТРИМАННЯ ОПИСУ ПОДІЇ ---
std::wstring GetEventDescription(EVENTLOGRECORD* pRecord, const std::wstring& sourceName) {
    HKEY hKey;
    std::wstring regPath = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" + sourceName;

    // Шукаємо шлях до DLL, яка містить тексти повідомлень
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        regPath = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\" + sourceName;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            regPath = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Security\\" + sourceName;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
                return L"Опис недоступний (Не знайдено EventMessageFile).";
            }
        }
    }

    wchar_t messageFile[MAX_PATH * 2] = { 0 };
    DWORD bufSize = sizeof(messageFile);
    if (RegGetValueW(hKey, NULL, L"EventMessageFile", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, NULL, messageFile, &bufSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return L"Опис недоступний (Помилка читання EventMessageFile).";
    }
    RegCloseKey(hKey);

    // Розгортаємо змінні оточення 
    wchar_t expandedFile[MAX_PATH * 2] = { 0 };
    ExpandEnvironmentStringsW(messageFile, expandedFile, MAX_PATH * 2);

    std::wstring dllPath = expandedFile;
    size_t semiColonPos = dllPath.find(L';');
    if (semiColonPos != std::wstring::npos) {
        dllPath = dllPath.substr(0, semiColonPos);
    }

    // Завантажуємо DLL з повідомленнями
    HMODULE hModule = LoadLibraryExW(dllPath.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!hModule) {
        return L"Опис недоступний (Не вдалося завантажити DLL повідомлень).";
    }

    // Збираємо аргументи (Strings), які вставляються в повідомлення
    std::vector<const wchar_t*> args;
    wchar_t* pString = (wchar_t*)((BYTE*)pRecord + pRecord->StringOffset);
    for (int i = 0; i < pRecord->NumStrings; ++i) {
        args.push_back(pString);
        pString += wcslen(pString) + 1;
    }

    // Форматуємо повідомлення
    wchar_t* pMessage = NULL;
    DWORD formatResult = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        hModule,
        pRecord->EventID,
        0, // Мова за замовчуванням
        (LPWSTR)&pMessage,
        0,
        (va_list*)args.data()
    );

    std::wstring result = L"";
    if (formatResult && pMessage) {
        result = pMessage;
        LocalFree(pMessage);
    }
    else {
        // Якщо не вдалося сформувати повідомлення, просто виводимо аргументи
        result = L"Опис не знайдено. Аргументи: ";
        for (const auto& arg : args) {
            result += arg;
            result += L" | ";
        }
    }

    FreeLibrary(hModule);

    // Видаляємо зайві перенесення рядків
    result.erase(std::remove(result.begin(), result.end(), L'\r'), result.end());
    result.erase(std::remove(result.begin(), result.end(), L'\n'), result.end());

    return result;
}

// --- ФУНКЦІЯ loadEventLogs ---
void SystemConfigurationGUI::loadEventLogs() {
    ui.tableWidget->setSortingEnabled(false);
    ui.tableWidget->setRowCount(0);
    ui.tableWidget->setColumnCount(7);

    // Динамічні заголовки
    if (isEnglish) {
        ui.tableWidget->setHorizontalHeaderLabels({ "Log Name", "Type", "Category", "Date/Time", "User", "Source", "Description" });
    }
    else {
        ui.tableWidget->setHorizontalHeaderLabels({
            QString::fromUtf8("Ім'я прот."), QString::fromUtf8("Тип події"), QString::fromUtf8("Категорія"),
            QString::fromUtf8("Дата створення"), QString::fromUtf8("Користувач"), QString::fromUtf8("Джерело"), QString::fromUtf8("Опис")
            });
    }

    struct EventRec {
        QString logName; QString type; QString category;
        QString dateTime; QString user; QString source; QString description;
        time_t rawTime;
    };
    std::vector<EventRec> allEvents;

    std::vector<std::wstring> logNames = { L"Application", L"Security", L"System", L"Setup" };

    for (const auto& logName : logNames) {
        HANDLE hLog = OpenEventLogW(NULL, logName.c_str());
        if (!hLog) continue;

        BYTE buffer[0x10000];
        DWORD bytesRead, bytesNeeded;
        int count = 0;
        int maxRecords = 500; // Зменшено до 500 для швидкості (читання описів займає час)

        while (ReadEventLogW(hLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ, 0,
            buffer, sizeof(buffer), &bytesRead, &bytesNeeded) && count < maxRecords) {

            DWORD offset = 0;
            while (offset < bytesRead && count < maxRecords) {
                EVENTLOGRECORD* pRecord = (EVENTLOGRECORD*)(buffer + offset);
                EventRec rec;

                // 1. Локалізація імені
                std::wstring origName(logName);
                if (isEnglish) {
                    rec.logName = QString::fromStdWString(origName);
                }
                else {
                    if (origName == L"Application") rec.logName = QString::fromUtf8("Додаток");
                    else if (origName == L"Security") rec.logName = QString::fromUtf8("Безпека");
                    else if (origName == L"System") rec.logName = QString::fromUtf8("Система");
                    else if (origName == L"Setup") rec.logName = QString::fromUtf8("Установка");
                    else rec.logName = QString::fromStdWString(origName);
                }

                // 2. Тип події
                if (pRecord->EventType == EVENTLOG_ERROR_TYPE) rec.type = isEnglish ? "Error" : QString::fromUtf8("Помилка");
                else if (pRecord->EventType == EVENTLOG_WARNING_TYPE) rec.type = isEnglish ? "Warning" : QString::fromUtf8("Увага");
                else if (pRecord->EventType == EVENTLOG_AUDIT_SUCCESS) rec.type = "Audit Success";
                else if (pRecord->EventType == EVENTLOG_AUDIT_FAILURE) rec.type = "Audit Failure";
                else rec.type = isEnglish ? "Info" : QString::fromUtf8("Інфо");

                rec.category = (pRecord->EventCategory != 0) ? QString::number(pRecord->EventCategory) : (isEnglish ? "None" : QString::fromUtf8("Немає"));

                // 3. Дата
                rec.rawTime = (time_t)pRecord->TimeGenerated;
                tm timeInfo;
                localtime_s(&timeInfo, &rec.rawTime);
                char timeBuf[64];
                strftime(timeBuf, 64, "%Y-%m-%d %H:%M:%S", &timeInfo);
                rec.dateTime = QString::fromUtf8(timeBuf);

                // 4. Користувач
                rec.user = " ";
                if (pRecord->UserSidLength > 0) {
                    PSID pSid = (PSID)((BYTE*)pRecord + pRecord->UserSidOffset);
                    wchar_t name[256], domain[256];
                    DWORD nameLen = 256, domainLen = 256;
                    SID_NAME_USE snu;
                    if (LookupAccountSidW(NULL, pSid, name, &nameLen, domain, &domainLen, &snu)) {
                        rec.user = QString::fromWCharArray(name);
                    }
                }
                else if (rec.logName == (isEnglish ? "System" : QString::fromUtf8("Система")) || rec.logName == (isEnglish ? "Security" : QString::fromUtf8("Безпека"))) {
                    rec.user = "System";
                }

                std::wstring sourceName = (wchar_t*)((BYTE*)pRecord + sizeof(EVENTLOGRECORD));
                rec.source = QString::fromStdWString(sourceName);

                // 5. Витягуємо ОПИС за допомогою нашої нової функції
                std::wstring description = GetEventDescription(pRecord, sourceName);
                rec.description = QString::fromStdWString(description);

                allEvents.push_back(rec);
                offset += pRecord->Length;
                count++;
            }
        }
        CloseEventLog(hLog);
    }

    std::sort(allEvents.begin(), allEvents.end(), [](const EventRec& a, const EventRec& b) {
        return a.rawTime > b.rawTime;
        });

    for (const auto& e : allEvents) {
        int r = ui.tableWidget->rowCount();
        ui.tableWidget->insertRow(r);

        // 0. Ім'я журналу
        ui.tableWidget->setItem(r, 0, new QTableWidgetItem(e.logName));

        // 1. ТИП ПОДІЇ З ІКОНКОЮ
        QTableWidgetItem* typeItem = new QTableWidgetItem(e.type);
        if (e.type == "Error" || e.type == QString::fromUtf8("Помилка")) {
            typeItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical));
        }
        else if (e.type == "Warning" || e.type == QString::fromUtf8("Увага")) {
            typeItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning));
        }
        else if (e.type.contains("Audit")) {
            // Для аудитів (успіх/відмова) поставимо іконку із замком або знаком питання
            typeItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion));
        }
        else {
            typeItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));
        }
        ui.tableWidget->setItem(r, 1, typeItem); // Додаємо налаштований елемент у таблицю

        // Решта колонок
        ui.tableWidget->setItem(r, 2, new QTableWidgetItem(e.category));
        ui.tableWidget->setItem(r, 3, new QTableWidgetItem(e.dateTime));
        ui.tableWidget->setItem(r, 4, new QTableWidgetItem(e.user));
        ui.tableWidget->setItem(r, 5, new QTableWidgetItem(e.source));

        QTableWidgetItem* descItem = new QTableWidgetItem(e.description);
        descItem->setToolTip(e.description); // Щоб при наведенні показувався повний текст
        ui.tableWidget->setItem(r, 6, descItem);
    }

    if (allEvents.empty()) {
        int r = ui.tableWidget->rowCount();
        ui.tableWidget->insertRow(r);
        ui.tableWidget->setItem(r, 0, new QTableWidgetItem(isEnglish ? "No logs found or access denied." : QString::fromUtf8("Немає доступу до журналів або вони порожні.")));
    }

    ui.tableWidget->setWordWrap(false);
    for (int i = 0; i < 6; ++i) ui.tableWidget->resizeColumnToContents(i);
    ui.tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui.tableWidget->setSortingEnabled(true);
}

void SystemConfigurationGUI::updateTreeLanguage() {
    QTreeWidgetItem* root = ui.treeWidget->topLevelItem(0); // Кореневий елемент "Конфігурація"
    if (!root) return;

    // =====================================================================
    // НОВЕ: ДОДАЄМО СИСТЕМНІ ІКОНКИ ДЛЯ КОЖНОГО ПУНКТУ
    // =====================================================================
    // Іконка комп'ютера для головного розділу
    root->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));

    // Іконки для підпунктів
    root->child(0)->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation)); // Регіональні (Інфо)
    root->child(1)->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileDialogListView)); // Оточення (Список)
    root->child(2)->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DriveNetIcon));       // Панель (Мережа/Система)
    root->child(3)->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_TrashIcon));          // Кошик (Справжній кошик)
    root->child(4)->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));           // Файли (Документ)
    root->child(5)->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));            // Папки (Жовта папка)
    root->child(6)->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning));  // Логи (Увага/Журнал)

    if (isEnglish) {
        root->setText(0, "Configuration");
        root->child(0)->setText(0, "Regional Settings");
        root->child(1)->setText(0, "Environment");
        root->child(2)->setText(0, "Control Panel");
        root->child(3)->setText(0, "Recycle Bin");
        root->child(4)->setText(0, "System Files");
        root->child(5)->setText(0, "System Folders");
        root->child(6)->setText(0, "Event Logs");
    }
    else {
        root->setText(0, QString::fromUtf8("Конфігурація"));
        root->child(0)->setText(0, QString::fromUtf8("Регіональні установки"));
        root->child(1)->setText(0, QString::fromUtf8("Оточення"));
        root->child(2)->setText(0, QString::fromUtf8("Панель керування"));
        root->child(3)->setText(0, QString::fromUtf8("Кошик"));
        root->child(4)->setText(0, QString::fromUtf8("Системні файли"));
        root->child(5)->setText(0, QString::fromUtf8("Системні папки"));
        root->child(6)->setText(0, QString::fromUtf8("Протоколи подій"));
    }
}

void SystemConfigurationGUI::loadRootMenu() {
    ui.listWidget->clear();

    // --- НАЛАШТУВАННЯ АКУРАТНОЇ СІТКИ ------
    ui.listWidget->setViewMode(QListWidget::IconMode);
    ui.listWidget->setIconSize(QSize(40, 40));
    ui.listWidget->setMovement(QListView::Static);
    ui.listWidget->setResizeMode(QListWidget::Adjust);

   
    ui.listWidget->setSpacing(15);
    ui.listWidget->setWordWrap(true);
    ui.listWidget->setTextElideMode(Qt::ElideNone); 
    // ------------------------------------

    // Список вкладок 
    struct Shortcut { QString nameUk; QString nameEn; QStyle::StandardPixmap icon; };
    std::vector<Shortcut> items = {
        { QString::fromUtf8("Регіональні\nустановки"), "Regional\nSettings", QStyle::SP_MessageBoxInformation },
        { QString::fromUtf8("Оточення"), "Environment", QStyle::SP_FileDialogListView },
        { QString::fromUtf8("Панель\nкерування"), "Control\nPanel", QStyle::SP_DriveNetIcon },
        { QString::fromUtf8("Кошик"), "Recycle\nBin", QStyle::SP_TrashIcon },
        { QString::fromUtf8("Системні\nфайли"), "System\nFiles", QStyle::SP_FileIcon },
        { QString::fromUtf8("Системні\nпапки"), "System\nFolders", QStyle::SP_DirIcon },
        { QString::fromUtf8("Протоколи\nподій"), "Event\nLogs", QStyle::SP_MessageBoxWarning }
    };

    for (const auto& s : items) {
        QString text = isEnglish ? s.nameEn : s.nameUk;
        QIcon icon = QApplication::style()->standardIcon(s.icon);

        QListWidgetItem* item = new QListWidgetItem(icon, text);
        item->setTextAlignment(Qt::AlignCenter); // Текст ідеально по центру
        ui.listWidget->addItem(item);
    }
}