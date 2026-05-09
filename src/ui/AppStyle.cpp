#include "AppStyle.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QDialog>
#include <QFile>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QStyle>
#include <QTextStream>
#include <QToolButton>
#include <QWidget>

QString AppStyle::readResource(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    return stream.readAll();
}

QString AppStyle::styleSheet()
{
    static const QString style = readResource(":/qss/app.qss")
            + QLatin1Char('\n') + readResource(":/qss/mainwindow.qss")
            + QLatin1Char('\n') + readResource(":/qss/dialogs.qss")
            + QLatin1Char('\n') + readResource(":/qss/tables.qss");
    return style;
}

void AppStyle::apply(QApplication *app)
{
    if (!app) {
        return;
    }

    app->setStyleSheet(styleSheet());
}

void AppStyle::apply(QWidget *widget, bool appendCurrentStyle)
{
    if (!widget) {
        return;
    }

    const QString appStyle = styleSheet();
    if (appendCurrentStyle && !widget->styleSheet().isEmpty()) {
        widget->setStyleSheet(widget->styleSheet() + QLatin1Char('\n') + appStyle);
    } else {
        widget->setStyleSheet(appStyle);
    }
}

void AppStyle::prepareMainWindow(QWidget *window)
{
    if (!window) {
        return;
    }

    window->setProperty("windowKind", "main");

    const auto buttons = window->findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        const QString name = button->objectName().toLower();
        if (button->property("role").isValid()) {
            continue;
        }

        if (name.contains("factory") || name.contains("reset") || name.contains("delete")) {
            button->setProperty("role", "danger");
        } else if (name.contains("search") || name.contains("login") || name.contains("config")
                   || name.contains("basic_config")) {
            button->setProperty("role", "primary");
        } else {
            button->setProperty("role", "secondary");
        }
    }

    const auto views = window->findChildren<QAbstractItemView *>();
    for (QAbstractItemView *view : views) {
        view->setAlternatingRowColors(true);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
    }

    const auto tables = window->findChildren<QTableView *>();
    for (QTableView *table : tables) {
        table->setShowGrid(false);
        if (table->horizontalHeader()) {
            table->horizontalHeader()->setHighlightSections(false);
        }
    }
}

void AppStyle::prepareDialog(QDialog *dialog)
{
    if (!dialog) {
        return;
    }

    dialog->setProperty("windowKind", "dialog");

    const auto buttons = dialog->findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        const QString text = button->text().toLower();
        if (button->property("role").isValid()) {
            continue;
        }

        if (text.contains("ok") || text.contains("确定") || text.contains("纭")) {
            button->setProperty("role", "primary");
        } else if (text.contains("delete") || text.contains("删除") || text.contains("鍒")) {
            button->setProperty("role", "danger");
        } else {
            button->setProperty("role", "secondary");
        }
    }

    const auto toolButtons = dialog->findChildren<QToolButton *>();
    for (QToolButton *button : toolButtons) {
        button->setProperty("role", "toolbar");
    }

    const auto views = dialog->findChildren<QAbstractItemView *>();
    for (QAbstractItemView *view : views) {
        view->setAlternatingRowColors(true);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
    }
}

void AppStyle::refresh(QWidget *widget)
{
    if (!widget) {
        return;
    }

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

void AppStyle::setInvalid(QWidget *widget, bool invalid)
{
    setState(widget, "invalid", invalid);
}

void AppStyle::setMuted(QWidget *widget, bool muted)
{
    setState(widget, "mutedText", muted);
}

void AppStyle::setState(QWidget *widget, const char *name, bool enabled)
{
    if (!widget) {
        return;
    }

    widget->setProperty(name, enabled);
    refresh(widget);
}
