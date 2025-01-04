/*
    SPDX-FileCopyrightText: 2018 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
    SPDX-FileCopyrightText: 2023 g10 Code GmbH
    SPDX-FileContributor: Sune Stolborg Vuorela <sune@vuorela.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "signaturepartutils.h"

#include "signaturepartutilsimageitemdelegate.h"
#include "signaturepartutilskconfig.h"
#include "signaturepartutilskeydelegate.h"
#include "signaturepartutilsmodel.h"
#include "signaturepartutilsrecentimagesmodel.h"

#include "core/document.h"
#include "core/form.h"
#include "core/page.h"
#include "pageview.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QInputDialog>
#include <QLabel>
#include <QListView>
#include <QMenu>
#include <QMimeDatabase>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "ui_selectcertificatedialog.h"
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
namespace
{
}

namespace SignaturePartUtils
{

std::optional<SigningInformation> getCertificateAndPasswordForSigning(PageView *pageView, Okular::Document *doc, SigningInformationOptions opts)
{
    const Okular::CertificateStore *certStore = doc->certificateStore();
    bool userCancelled, nonDateValidCerts;
    QList<Okular::CertificateInfo> certs = certStore->signingCertificatesForNow(&userCancelled, &nonDateValidCerts);
    if (userCancelled) {
        return std::nullopt;
    }

    if (certs.isEmpty()) {
        pageView->showNoSigningCertificatesDialog(nonDateValidCerts);
        return std::nullopt;
    }
    QString password;
    QString documentPassword;

    QStandardItemModel items;
    QHash<QString, Okular::CertificateInfo> nickToCert;
    qsizetype minWidth = -1;
    bool showIcons = false;
    int selectIndex = 0;
    auto config = KSharedConfig::openConfig();
    const QString lastNick = config->group(ConfigGroup()).readEntry<QString>(ConfigLastKeyNick(), QString());
    for (const auto &cert : std::as_const(certs)) {
        auto item = std::make_unique<QStandardItem>();
        QString commonName = cert.subjectInfo(Okular::CertificateInfo::CommonName, Okular::CertificateInfo::EmptyString::Empty);
        item->setData(commonName, CommonNameRole);
        QString emailAddress = cert.subjectInfo(Okular::CertificateInfo::EmailAddress, Okular::CertificateInfo::EmptyString::Empty);
        item->setData(emailAddress, EmailRole);

        minWidth = std::max(minWidth, std::max(cert.nickName().size(), emailAddress.size() + commonName.size()));

        switch (cert.keyLocation()) {
        case Okular::CertificateInfo::KeyLocation::Computer:
            item->setData(QIcon::fromTheme(QStringLiteral("view-certificate")), Qt::DecorationRole);
            showIcons = true;
            break;
        case Okular::CertificateInfo::KeyLocation::HardwareToken:
            /* Better icon requested in https://bugs.kde.org/show_bug.cgi?id=428278*/
            item->setData(QIcon::fromTheme(QStringLiteral("auth-sim")), Qt::DecorationRole);
            showIcons = true;
            break;
        case Okular::CertificateInfo::KeyLocation::Unknown:; //
            break;
        case Okular::CertificateInfo::KeyLocation::Other:
            break;
        }

        QString nick = cert.nickName();
        item->setData(nick, NickRole);

        if (cert.backend() == Okular::CertificateInfo::Backend::Gpg) {
            static const auto group4 = QRegularExpression(QStringLiteral("(....)"));
            nick = nick.replace(group4, QStringLiteral("\\1 ")).trimmed();
        }
        item->setData(nick, NickDisplayRole);
        item->setData(cert.subjectInfo(Okular::CertificateInfo::DistinguishedName, Okular::CertificateInfo::EmptyString::Empty), Qt::ToolTipRole);
        item->setEditable(false);
        if (cert.nickName() == lastNick) {
            selectIndex = items.rowCount();
        }
        items.appendRow(item.release());
        nickToCert[cert.nickName()] = cert;
    }

    SelectCertificateDialog dialog(pageView);
    auto keyDelegate = new KeyDelegate(dialog.ui->list);
    keyDelegate->showIcon = showIcons;
    dialog.ui->list->setItemDelegate(keyDelegate);
    QFontMetrics fm = dialog.fontMetrics();
    dialog.ui->list->setMinimumWidth(fm.averageCharWidth() * (minWidth + 5));
    dialog.ui->list->setModel(&items);
    dialog.ui->list->setCurrentIndex(items.index(selectIndex, 0));
    if (items.rowCount() < 3) {
        auto rowHeight = dialog.ui->list->sizeHintForRow(0);
        dialog.ui->list->setFixedHeight(rowHeight * items.rowCount() + (items.rowCount() - 1) * dialog.ui->list->spacing() + dialog.ui->list->contentsMargins().top() + dialog.ui->list->contentsMargins().bottom());
    }
    QObject::connect(dialog.ui->list->selectionModel(), &QItemSelectionModel::selectionChanged, &dialog, [dialog = &dialog](auto &&, auto &&) {
        // One can ctrl-click on the selected item to deselect it, that would
        // leave the selection empty, so better prevent the OK button
        // from being usable
        dialog->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(dialog->ui->list->selectionModel()->hasSelection());
    });

    RecentImagesModel imagesModel;
    if (!(opts & SigningInformationOption::BackgroundImage)) {
        dialog.ui->backgroundInput->hide();
        dialog.ui->backgroundLabel->hide();
        dialog.ui->recentBackgrounds->hide();
        dialog.ui->recentLabel->hide();
        dialog.ui->backgroundButton->hide();
    } else {
        dialog.ui->recentBackgrounds->setModel(&imagesModel);
        dialog.ui->recentBackgrounds->setSelectionMode(QAbstractItemView::SingleSelection);
        dialog.ui->recentBackgrounds->setItemDelegate(new ImageItemDelegate);
        dialog.ui->recentBackgrounds->setViewMode(QListView::IconMode);
        dialog.ui->recentBackgrounds->setDragEnabled(false);
        dialog.ui->recentBackgrounds->setSpacing(3);
        dialog.ui->recentBackgrounds->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(dialog.ui->recentBackgrounds, &QListView::activated, &dialog, [&lineEdit = dialog.ui->backgroundInput](const QModelIndex &idx) { lineEdit->setText(idx.data(Qt::DisplayRole).toString()); });
        const bool haveRecent = imagesModel.rowCount(QModelIndex()) != 0;
        if (!haveRecent) {
            dialog.ui->recentBackgrounds->hide();
            dialog.ui->recentLabel->hide();
            QObject::connect(&imagesModel, &QAbstractItemModel::rowsInserted, &dialog, [&dialog]() {
                dialog.ui->recentBackgrounds->show();
                dialog.ui->recentLabel->show();
            });
        }

        QObject::connect(dialog.ui->backgroundInput, &QLineEdit::textChanged, &dialog, [recentModel = &imagesModel, selectionModel = dialog.ui->recentBackgrounds->selectionModel()](const QString &newText) {
            recentModel->setFileSystemSelection(newText);
            /*Update selection*/
            for (int row = 0; row < recentModel->rowCount(); row++) {
                const auto index = recentModel->index(row, 0);
                if (index.data().toString() == newText) {
                    selectionModel->select(index, QItemSelectionModel::ClearAndSelect);
                    break;
                }
            }
        });
        // needs to happen after textChanged connection on backgroundInput
        if (haveRecent) {
            dialog.ui->backgroundInput->setText(imagesModel.index(0, 0).data().toString());
        }

        QObject::connect(dialog.ui->backgroundButton, &QPushButton::clicked, &dialog, [lineEdit = dialog.ui->backgroundInput]() {
            const auto supportedFormats = QImageReader::supportedImageFormats();
            QString formats;
            for (const auto &format : supportedFormats) {
                if (!formats.isEmpty()) {
                    formats += QLatin1Char(' ');
                }
                formats += QStringLiteral("*.") + QString::fromUtf8(format);
            }
            const QString imageFormats = i18nc("file types in a file open dialog", "Images (%1)", formats);
            const QString filename = QFileDialog::getOpenFileName(lineEdit, i18n("Select background image"), QDir::homePath(), imageFormats);
            lineEdit->setText(filename);
        });
        QObject::connect(dialog.ui->recentBackgrounds, &QWidget::customContextMenuRequested, &dialog, [recentModel = &imagesModel, view = dialog.ui->recentBackgrounds](QPoint pos) {
            auto current = view->indexAt(pos);
            QAction currentImage(i18n("Forget image"));
            QAction allImages(i18n("Forget all images"));
            QList<QAction *> actions;
            if (current.isValid()) {
                actions.append(&currentImage);
            }
            if (recentModel->rowCount() > 1 || actions.empty()) {
                actions.append(&allImages);
            }
            const QAction *selected = QMenu::exec(actions, view->viewport()->mapToGlobal(pos), nullptr, view);
            if (selected == &currentImage) {
                recentModel->removeItem(current.data(Qt::DisplayRole).toString());
                recentModel->saveBack();
            } else if (selected == &allImages) {
                recentModel->clear();
                recentModel->saveBack();
            }
        });
    }
    dialog.ui->reasonInput->setText(config->group(ConfigGroup()).readEntry(ConfigLastReason(), QString()));
    dialog.ui->locationInput->setText(config->group(ConfigGroup()).readEntry(ConfigLastLocation(), QString()));
    auto result = dialog.exec();

    if (result == QDialog::Rejected) {
        return std::nullopt;
    }
    const auto certNicknameToUse = dialog.ui->list->currentIndex().data(NickRole).toString();
    auto backGroundImage = dialog.ui->backgroundInput->text();
    if (!backGroundImage.isEmpty()) {
        if (QFile::exists(backGroundImage)) {
            imagesModel.setFileSystemSelection(backGroundImage);
            imagesModel.saveBack();
        } else {
            // no need to send a nonworking image anywhere
            backGroundImage.clear();
        }
    }

    // I could not find any case in which i need to enter a password to use the certificate, seems that once you unlcok the firefox/NSS database
    // you don't need a password anymore, but still there's code to do that in NSS so we have code to ask for it if needed. What we do is
    // ask if the empty password is fine, if it is we don't ask the user anything, if it's not, we ask for a password
    Okular::CertificateInfo cert = nickToCert.value(certNicknameToUse);
    bool passok = cert.checkPassword(password);
    while (!passok) {
        const QString title = i18n("Enter password (if any) to unlock certificate: %1", certNicknameToUse);
        bool ok;
        password = QInputDialog::getText(pageView, i18n("Enter certificate password"), title, QLineEdit::Password, QString(), &ok);
        if (ok) {
            passok = cert.checkPassword(password);
        } else {
            passok = false;
            break;
        }
    }
    if (!passok) {
        return std::nullopt;
    }

    if (doc->metaData(QStringLiteral("DocumentHasPassword")).toString() == QLatin1String("yes")) {
        documentPassword = QInputDialog::getText(pageView, i18n("Enter document password"), i18n("Enter document password"), QLineEdit::Password, QString(), &passok);
    }

    if (passok) {
        config->group(ConfigGroup()).writeEntry(ConfigLastKeyNick(), cert.nickName());
        config->group(ConfigGroup()).writeEntry(ConfigLastReason(), dialog.ui->reasonInput->text());
        config->group(ConfigGroup()).writeEntry(ConfigLastLocation(), dialog.ui->locationInput->text());
        return SigningInformation {std::make_unique<Okular::CertificateInfo>(std::move(cert)), password, documentPassword, dialog.ui->reasonInput->text(), dialog.ui->locationInput->text(), backGroundImage};
    }
    return std::nullopt;
}

QString getFileNameForNewSignedFile(PageView *pageView, Okular::Document *doc)
{
    QMimeDatabase db;
    const QString typeName = doc->documentInfo().get(Okular::DocumentInfo::MimeType);
    const QMimeType mimeType = db.mimeTypeForName(typeName);
    const QString mimeTypeFilter = i18nc("File type name and pattern", "%1 (%2)", mimeType.comment(), mimeType.globPatterns().join(QLatin1Char(' ')));

    const QUrl currentFileUrl = doc->currentDocument();
    const QString localFilePathIfAny = currentFileUrl.isLocalFile() ? QFileInfo(currentFileUrl.path()).canonicalPath() + QLatin1Char('/') : QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString newFileName = localFilePathIfAny + getSuggestedFileNameForSignedFile(currentFileUrl.fileName(), mimeType.preferredSuffix());

    return QFileDialog::getSaveFileName(pageView, i18n("Save Signed File As"), newFileName, mimeTypeFilter);
}

void signUnsignedSignature(const Okular::FormFieldSignature *form, PageView *pageView, Okular::Document *doc)
{
    Q_ASSERT(form && form->signatureType() == Okular::FormFieldSignature::UnsignedSignature);
    const std::optional<SigningInformation> signingInfo = getCertificateAndPasswordForSigning(pageView, doc, SigningInformationOption::None);
    if (!signingInfo) {
        return;
    }

    Okular::NewSignatureData data;
    data.setCertNickname(signingInfo->certificate->nickName());
    data.setCertSubjectCommonName(signingInfo->certificate->subjectInfo(Okular::CertificateInfo::CommonName, Okular::CertificateInfo::EmptyString::TranslatedNotAvailable));
    data.setPassword(signingInfo->certificatePassword);
    data.setDocumentPassword(signingInfo->documentPassword);
    data.setReason(signingInfo->reason);
    data.setLocation(signingInfo->location);

    const QString newFilePath = getFileNameForNewSignedFile(pageView, doc);

    if (!newFilePath.isEmpty()) {
        const bool success = form->sign(data, newFilePath);
        if (success) {
            Q_EMIT pageView->requestOpenNewlySignedFile(newFilePath, form->page()->number() + 1);
        } else {
            KMessageBox::error(pageView, i18nc("%1 is a file path", "Could not sign. Invalid certificate password or could not write to '%1'", newFilePath));
        }
    }
}

SelectCertificateDialog::SelectCertificateDialog(QWidget *parent)
    : QDialog(parent)
    , ui {std::make_unique<Ui_SelectCertificateDialog>()}
{
    ui->setupUi(this);
}
SelectCertificateDialog::~SelectCertificateDialog() = default;
}
