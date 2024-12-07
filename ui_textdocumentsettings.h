#include <klocalizedstring.h>

/********************************************************************************
** Form generated from reading UI file 'textdocumentsettings.ui'
**
** Created by: Qt User Interface Compiler version 5.15.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TEXTDOCUMENTSETTINGS_H
#define UI_TEXTDOCUMENTSETTINGS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TextDocumentSettings
{
public:
    QFormLayout *formLayout;

    void setupUi(QWidget *TextDocumentSettings)
    {
        if (TextDocumentSettings->objectName().isEmpty())
            TextDocumentSettings->setObjectName(QString::fromUtf8("TextDocumentSettings"));
        formLayout = new QFormLayout(TextDocumentSettings);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

        retranslateUi(TextDocumentSettings);

        QMetaObject::connectSlotsByName(TextDocumentSettings);
    } // setupUi

    void retranslateUi(QWidget *TextDocumentSettings)
    {
        (void)TextDocumentSettings;
    } // retranslateUi

};

namespace Ui {
    class TextDocumentSettings: public Ui_TextDocumentSettings {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TEXTDOCUMENTSETTINGS_H

