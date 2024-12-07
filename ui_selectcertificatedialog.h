#include <klocalizedstring.h>

/********************************************************************************
** Form generated from reading UI file 'selectcertificatedialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SELECTCERTIFICATEDIALOG_H
#define UI_SELECTCERTIFICATEDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_SelectCertificateDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QListView *list;
    QFormLayout *formLayout;
    QLabel *reasonLabel;
    QLineEdit *reasonInput;
    QLabel *locationLabel;
    QLineEdit *locationInput;
    QLabel *backgroundLabel;
    QHBoxLayout *horizontalLayout;
    QLineEdit *backgroundInput;
    QPushButton *backgroundButton;
    QLabel *recentLabel;
    QListView *recentBackgrounds;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *SelectCertificateDialog)
    {
        if (SelectCertificateDialog->objectName().isEmpty())
            SelectCertificateDialog->setObjectName(QString::fromUtf8("SelectCertificateDialog"));
        SelectCertificateDialog->resize(400, 659);
        verticalLayout = new QVBoxLayout(SelectCertificateDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        label = new QLabel(SelectCertificateDialog);
        label->setObjectName(QString::fromUtf8("label"));

        verticalLayout->addWidget(label);

        list = new QListView(SelectCertificateDialog);
        list->setObjectName(QString::fromUtf8("list"));
        list->setAlternatingRowColors(true);
        list->setSelectionBehavior(QAbstractItemView::SelectRows);
        list->setLayoutMode(QListView::SinglePass);

        verticalLayout->addWidget(list);

        formLayout = new QFormLayout();
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        reasonLabel = new QLabel(SelectCertificateDialog);
        reasonLabel->setObjectName(QString::fromUtf8("reasonLabel"));

        formLayout->setWidget(0, QFormLayout::LabelRole, reasonLabel);

        reasonInput = new QLineEdit(SelectCertificateDialog);
        reasonInput->setObjectName(QString::fromUtf8("reasonInput"));
        reasonInput->setClearButtonEnabled(true);

        formLayout->setWidget(0, QFormLayout::FieldRole, reasonInput);

        locationLabel = new QLabel(SelectCertificateDialog);
        locationLabel->setObjectName(QString::fromUtf8("locationLabel"));

        formLayout->setWidget(1, QFormLayout::LabelRole, locationLabel);

        locationInput = new QLineEdit(SelectCertificateDialog);
        locationInput->setObjectName(QString::fromUtf8("locationInput"));
        locationInput->setClearButtonEnabled(true);

        formLayout->setWidget(1, QFormLayout::FieldRole, locationInput);

        backgroundLabel = new QLabel(SelectCertificateDialog);
        backgroundLabel->setObjectName(QString::fromUtf8("backgroundLabel"));

        formLayout->setWidget(2, QFormLayout::LabelRole, backgroundLabel);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        backgroundInput = new QLineEdit(SelectCertificateDialog);
        backgroundInput->setObjectName(QString::fromUtf8("backgroundInput"));
        backgroundInput->setClearButtonEnabled(true);

        horizontalLayout->addWidget(backgroundInput);

        backgroundButton = new QPushButton(SelectCertificateDialog);
        backgroundButton->setObjectName(QString::fromUtf8("backgroundButton"));
        QIcon icon(QIcon::fromTheme(QString::fromUtf8("folder-image")));
        backgroundButton->setIcon(icon);

        horizontalLayout->addWidget(backgroundButton);


        formLayout->setLayout(2, QFormLayout::FieldRole, horizontalLayout);

        recentLabel = new QLabel(SelectCertificateDialog);
        recentLabel->setObjectName(QString::fromUtf8("recentLabel"));
        recentLabel->setTextFormat(Qt::AutoText);
        recentLabel->setWordWrap(false);

        formLayout->setWidget(3, QFormLayout::LabelRole, recentLabel);

        recentBackgrounds = new QListView(SelectCertificateDialog);
        recentBackgrounds->setObjectName(QString::fromUtf8("recentBackgrounds"));

        formLayout->setWidget(3, QFormLayout::FieldRole, recentBackgrounds);


        verticalLayout->addLayout(formLayout);

        verticalSpacer = new QSpacerItem(20, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(SelectCertificateDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(SelectCertificateDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), SelectCertificateDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), SelectCertificateDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(SelectCertificateDialog);
    } // setupUi

    void retranslateUi(QDialog *SelectCertificateDialog)
    {
        SelectCertificateDialog->setWindowTitle(tr2i18n("Certificates", nullptr));
        label->setText(tr2i18n("Select certificate to sign with:", nullptr));
        reasonLabel->setText(tr2i18n("Reason:", nullptr));
        reasonInput->setPlaceholderText(tr2i18n("optional", nullptr));
        locationLabel->setText(tr2i18n("Location:", nullptr));
        locationInput->setPlaceholderText(tr2i18n("optional", nullptr));
        backgroundLabel->setText(tr2i18n("Background:", nullptr));
        backgroundInput->setPlaceholderText(tr2i18n("optional", nullptr));
        backgroundButton->setText(tr2i18n("Choose...", nullptr));
        recentLabel->setText(tr2i18n("Recent backgrounds:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SelectCertificateDialog: public Ui_SelectCertificateDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // SELECTCERTIFICATEDIALOG_H

