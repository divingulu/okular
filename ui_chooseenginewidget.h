#include <klocalizedstring.h>

/********************************************************************************
** Form generated from reading UI file 'chooseenginewidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHOOSEENGINEWIDGET_H
#define UI_CHOOSEENGINEWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ChooseEngineWidget
{
public:
    QVBoxLayout *vboxLayout;
    QLabel *description;
    QComboBox *engineList;

    void setupUi(QWidget *ChooseEngineWidget)
    {
        if (ChooseEngineWidget->objectName().isEmpty())
            ChooseEngineWidget->setObjectName(QString::fromUtf8("ChooseEngineWidget"));
        ChooseEngineWidget->resize(286, 96);
        vboxLayout = new QVBoxLayout(ChooseEngineWidget);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        description = new QLabel(ChooseEngineWidget);
        description->setObjectName(QString::fromUtf8("description"));
        description->setTextFormat(Qt::RichText);

        vboxLayout->addWidget(description);

        engineList = new QComboBox(ChooseEngineWidget);
        engineList->setObjectName(QString::fromUtf8("engineList"));

        vboxLayout->addWidget(engineList);


        retranslateUi(ChooseEngineWidget);

        QMetaObject::connectSlotsByName(ChooseEngineWidget);
    } // setupUi

    void retranslateUi(QWidget *ChooseEngineWidget)
    {
        description->setText(QString());
        (void)ChooseEngineWidget;
    } // retranslateUi

};

namespace Ui {
    class ChooseEngineWidget: public Ui_ChooseEngineWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CHOOSEENGINEWIDGET_H

