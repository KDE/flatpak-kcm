// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

#include <QDebug>
#include <QMetaEnum>
#include <QTest>

#include <KLocalizedString>

#include "flatpakpermission.h"

class FlatpakPermissionModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init()
    {
        QDir().rmdir(QFINDTESTDATA("fixtures/overrides/"));
    }

    void testRead()
    {
        // The primary motivation behind this test is to make sure that translations aren't being pulled in for the raw names.
        FlatpakReferencesModel referencesModel;
        QFile metadataFile(QFINDTESTDATA("fixtures/metadata/com.discordapp.Discord"));
        QVERIFY(metadataFile.open(QFile::ReadOnly));
        FlatpakReference reference(&referencesModel,
                                   "com.discordapp.Discord",
                                   "x86_64",
                                   "stable",
                                   "0.0.24",
                                   "Discord",
                                   QFINDTESTDATA("fixtures/overrides/"),
                                   QUrl(),
                                   metadataFile.readAll());
        FlatpakPermissionModel model;
        model.setReference(&reference);
        model.load();
        bool containsNetwork = false;
        bool containsXdgDownload = false;
        for (auto i = 0; i <= model.rowCount(); ++i) {
            const QString name = model.data(model.index(i, 0), FlatpakPermissionModel::Name).toString();
            if (name == "network") {
                containsNetwork = true;
            }

            if (name == "xdg-download") {
                containsXdgDownload = true;
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::IsGranted), true);
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::EffectiveValue), i18n("read/write"));
            }
        }

        QVERIFY(containsNetwork);
        QVERIFY(containsXdgDownload);
        QVERIFY(model.permExists("network"));
        QVERIFY(!model.permExists("yolo-foobar"));
    }

    void testMutable()
    {
        // Ensure override files mutate properly
        FlatpakReferencesModel referencesModel;
        QFile metadataFile(QFINDTESTDATA("fixtures/metadata/com.discordapp.Discord"));
        QVERIFY(metadataFile.open(QFile::ReadOnly));
        FlatpakReference reference(&referencesModel,
                                   "com.discordapp.Discord",
                                   "x86_64",
                                   "stable",
                                   "0.0.24",
                                   "Discord",
                                   QFINDTESTDATA("fixtures/overrides/"),
                                   QUrl(),
                                   metadataFile.readAll());
        FlatpakPermissionModel model;
        model.setReference(&reference);
        model.load();
        model.setShowAdvanced(true);
        for (auto i = 0; i <= model.rowCount(); ++i) {
            const QString name = model.data(model.index(i, 0), FlatpakPermissionModel::Name).toString();
            if (name == "host") {
                const auto metaEnum = QMetaEnum::fromType<FlatpakPermissionModel::Roles>();
                for (auto j = 0; j <= metaEnum.keyCount(); ++j) { // purely for debugging purposes
                    qDebug() << metaEnum.key(j) << model.data(model.index(i, 0), metaEnum.value(j));
                }

                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::IsGranted), false);
                model.togglePermissionAtIndex(i);
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::IsGranted), true);
            }

            if (name == "org.kde.StatusNotifierWatcher") {
                model.editPerm(i, i18n("own"));
            }

            if (name == "host-os") {
                // Make sure the config manipulation works across multiple changes
                model.editPerm(i, i18n("read-only"));
                model.editPerm(i, i18n("read-write"));
                model.editPerm(i, i18n("create"));
                model.editPerm(i, i18n("read-only"));
            }
        }
        model.save();
        QFile actualFile(QFINDTESTDATA("fixtures/overrides/com.discordapp.Discord"));
        QVERIFY(actualFile.open(QFile::ReadOnly));
        QFile expectedFile(QFINDTESTDATA("fixtures/overrides.out/com.discordapp.Discord"));
        QVERIFY(expectedFile.open(QFile::ReadOnly));
        QCOMPARE(actualFile.readAll(), expectedFile.readAll());
    }
};

QTEST_MAIN(FlatpakPermissionModelTest)

#include "flatpakpermissiontest.moc"
