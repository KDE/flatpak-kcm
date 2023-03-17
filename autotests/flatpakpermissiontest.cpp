// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

#include <QDebug>
#include <QMetaEnum>
#include <QTest>

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include "flatpakcommon.h"
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
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::IsEffectiveEnabled), true);
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::EffectiveValue), i18n("read/write"));
            }
        }

        QVERIFY(containsNetwork);
        QVERIFY(containsXdgDownload);
        QVERIFY(model.permExists("network"));
        QVERIFY(!model.permExists("yolo-foobar"));
    }

    void testDefaultFilesystemsGoFirst()
    {
        // If there are no custom filesystems specified in defaults, then all custom ones should go below it.
        // The Discord test above can't be reused, because it has custom filesystems in base metadata, so
        // the well-known "host-etc" filesystem would not be the last one. But we want to test for the last
        // default index too.
        FlatpakReferencesModel referencesModel;
        QFile metadataFile(QFINDTESTDATA("fixtures/metadata/org.gnome.dfeet"));
        QVERIFY(metadataFile.open(QFile::ReadOnly));
        FlatpakReference reference(&referencesModel,
                                   "org.gnome.dfeet",
                                   "x86_64",
                                   "stable",
                                   "0.3.16",
                                   "D-Feet",
                                   QFINDTESTDATA("fixtures/overrides/"),
                                   QUrl(),
                                   metadataFile.readAll());
        FlatpakPermissionModel model;
        model.setReference(&reference);
        model.load();
        QStringList filesystems;
        for (auto i = 0; i <= model.rowCount(); ++i) {
            const QString name = model.data(model.index(i, 0), FlatpakPermissionModel::Name).toString();
            // collect all filesystems
            const QString categoryi18n = model.data(model.index(i, 0), FlatpakPermissionModel::Category).toString();
            const QString category = FlatpakPermission::categoryHeadingToRawCategory(categoryi18n);
            if (category == QLatin1String(FLATPAK_METADATA_KEY_FILESYSTEMS)) {
                filesystems.append(name);
            }
        }

        // Note: update this name when more standard filesystems are added.
        const QString hostEtc = QLatin1String("host-etc");
        const auto indexOfHostEtc = filesystems.indexOf(hostEtc);
        QVERIFY(indexOfHostEtc == 3);
        // But custom overrides should come after standard ones anyway.
        const auto custom = QLatin1String("/custom/path");
        const auto indexOfCustom = filesystems.indexOf(custom);
        QVERIFY(indexOfCustom != -1);
        QVERIFY(indexOfHostEtc < indexOfCustom);
    }

    void testDBusNonePolicy()
    {
        // This test verifies that "none" policy for D-Bus services works just
        // like other regular policies. It can be loaded, changed to, changed
        // from, and saved.
        FlatpakReferencesModel referencesModel;
        QFile metadataFile(QFINDTESTDATA("fixtures/metadata/org.gnome.dfeet"));
        QVERIFY(metadataFile.open(QFile::ReadOnly));
        FlatpakReference reference(&referencesModel,
                                   "org.gnome.dfeet",
                                   "x86_64",
                                   "stable",
                                   "0.3.16",
                                   "D-Feet",
                                   QFINDTESTDATA("fixtures/overrides/"),
                                   QUrl(),
                                   metadataFile.readAll());
        FlatpakPermissionModel model;
        model.setReference(&reference);
        model.load();
        model.setShowAdvanced(true);

        // This service is set to "none" by default (in metadata).
        const auto service0 = QLatin1String("com.example.service0");
        int indexOfService0 = -1;
        // This service is set to "none" in override file.
        const auto service1 = QLatin1String("com.example.service1");
        int indexOfService1 = -1;
        // This service is set to "see" in override, but will be changed to "none".
        const auto service2 = QLatin1String("com.example.service2");
        int indexOfService2 = -1;

        for (auto i = 0; i <= model.rowCount(); ++i) {
            const QString name = model.data(model.index(i, 0), FlatpakPermissionModel::Name).toString();
            const QString categoryi18n = model.data(model.index(i, 0), FlatpakPermissionModel::Category).toString();
            const QString category = FlatpakPermission::categoryHeadingToRawCategory(categoryi18n);
            if (category == QLatin1String(FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY)) {
                if (name == service0) {
                    indexOfService0 = i;
                } else if (name == service1) {
                    indexOfService1 = i;
                } else if (name == service2) {
                    indexOfService2 = i;
                }
            }
        }
        QVERIFY(indexOfService0 != -1);
        QVERIFY(indexOfService1 != -1);
        QVERIFY(indexOfService2 != -1);
        const auto name0 = model.data(model.index(indexOfService0, 0), FlatpakPermissionModel::EffectiveValue).toString();
        const auto name1 = model.data(model.index(indexOfService1, 0), FlatpakPermissionModel::EffectiveValue).toString();
        const auto name2 = model.data(model.index(indexOfService2, 0), FlatpakPermissionModel::EffectiveValue).toString();
        QCOMPARE(name0, i18n("None"));
        QCOMPARE(name1, i18n("None"));
        QCOMPARE(name2, i18n("see"));

        const auto setAndCheckBus = [&](const QString &value, const QString &i18nValue) {
            model.editPerm(indexOfService2, i18nValue);
            const auto name2i18nValue = model.data(model.index(indexOfService2, 0), FlatpakPermissionModel::EffectiveValue).toString();
            QCOMPARE(name2i18nValue, i18nValue);

            QVERIFY(model.isSaveNeeded());
            model.save();

            const KConfig expectedDesktopFile(QFINDTESTDATA("fixtures/overrides/org.gnome.dfeet"));
            const auto group = expectedDesktopFile.group(QLatin1String(FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY));
            const auto name2value = group.readEntry(service2);
            QCOMPARE(name2value, value);
        };

        setAndCheckBus(QLatin1String("none"), i18n("None"));
        setAndCheckBus(QLatin1String("see"), i18n("see"));
        setAndCheckBus(QLatin1String("talk"), i18n("talk"));
        setAndCheckBus(QLatin1String("own"), i18n("own"));

        const auto checkPossibleValues = [&](int row) {
            const auto values = model.data(model.index(row, 0), FlatpakPermissionModel::ValueList).toStringList();
            QVERIFY(values.contains(i18n("None")));
            QVERIFY(values.contains(i18n("see")));
            QVERIFY(values.contains(i18n("talk")));
            QVERIFY(values.contains(i18n("own")));
        };
        checkPossibleValues(indexOfService0);
        checkPossibleValues(indexOfService1);
        checkPossibleValues(indexOfService2);
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

                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::IsEffectiveEnabled), false);
                model.togglePermissionAtIndex(i);
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::IsEffectiveEnabled), true);
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
