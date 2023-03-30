// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

#include <QAbstractItemModel>
#include <QDebug>
#include <QMetaEnum>
#include <QTest>

#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <array>

#include "flatpakcommon.h"
#include "flatpakpermission.h"

class FlatpakPermissionModelTest : public QObject
{
    Q_OBJECT
private:
    /** KConfig comparison with a grain of Flatpak specifics. */
    bool operatorFlatpakConfigEquals(const KConfig &actual, const KConfig &expected)
    {
        auto actualGroups = actual.groupList();
        actualGroups.sort();
        auto expectedGroups = expected.groupList();
        expectedGroups.sort();

        if (actualGroups != expectedGroups) {
            return false;
        }

        for (const auto &group : std::as_const(actualGroups)) {
            const auto actualGroup = actual.group(group);
            const auto expectedGroup = expected.group(group);

            auto actualKeys = actualGroup.keyList();
            actualKeys.sort();
            auto expectedKeys = expectedGroup.keyList();
            expectedKeys.sort();

            if (actualKeys != expectedKeys) {
                return false;
            }

            for (const auto &key : std::as_const(actualKeys)) {
                if (group == QLatin1String(FLATPAK_METADATA_GROUP_CONTEXT)) {
                    // Treat everything in [Context] as XDG lists
                    auto actualValues = actualGroup.readXdgListEntry(key);
                    actualValues.sort();
                    auto expectedValues = expectedGroup.readXdgListEntry(key);
                    expectedValues.sort();

                    if (actualValues != expectedValues) {
                        return false;
                    }
                } else {
                    auto actualValue = actualGroup.readEntry(key);
                    auto expectedValue = expectedGroup.readEntry(key);

                    if (actualValue != expectedValue) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool modelContains(const QAbstractItemModel *model, const QVariant &value, int role = Qt::DisplayRole)
    {
        for (int i = 0; i < model->rowCount(); i++) {
            const auto &data = model->data(model->index(i, 0), role);
            if (data == value) {
                return true;
            }
        }
        return false;
    }

private Q_SLOTS:
    void init()
    {
        QDir().rmdir(QFINDTESTDATA("fixtures/overrides/"));
    }

    void testCompareFlatpakConfigsEqual()
    {
        KConfig configA(QString(), KConfig::SimpleConfig);
        KConfig configB(QString(), KConfig::SimpleConfig);

        const std::array configs = {&configA, &configB};
        for (const auto &config : configs) {
            auto group1 = config->group(QLatin1String(FLATPAK_METADATA_GROUP_ENVIRONMENT));
            auto group2 = config->group(QLatin1String(FLATPAK_METADATA_GROUP_CONTEXT));

            group1.writeEntry(QStringLiteral("key1"), QStringLiteral("value1"));
            group1.writeEntry(QStringLiteral("key2"), QStringLiteral("value2"));

            auto values = QStringList{QStringLiteral("itemA"), QStringLiteral("itemB")};
            group2.writeEntry(QStringLiteral("key3"), values);
            // Reorder list entries
            if (config == &configB) {
                values = QStringList{QStringLiteral("itemC"), QStringLiteral("itemD")};
            } else {
                values = QStringList{QStringLiteral("itemD"), QStringLiteral("itemC")};
            }
            group2.writeXdgListEntry(QStringLiteral("key4"), values);
        }

        QVERIFY(operatorFlatpakConfigEquals(configA, configB));
    }

    void testCompareFlatpakConfigsWhereXDGListsDiffer()
    {
        KConfig configA(QString(), KConfig::SimpleConfig);
        KConfig configB(QString(), KConfig::SimpleConfig);

        auto groupA = configA.group(QLatin1String(FLATPAK_METADATA_GROUP_CONTEXT));
        auto groupB = configB.group(QLatin1String(FLATPAK_METADATA_GROUP_CONTEXT));

        QStringList values = QStringList{QStringLiteral("itemA"), QStringLiteral("itemB")};
        groupA.writeXdgListEntry(QStringLiteral("key4"), values);
        values.append(QStringLiteral("itemC"));
        groupB.writeXdgListEntry(QStringLiteral("key4"), values);

        QVERIFY(!operatorFlatpakConfigEquals(configA, configB));
    }

    void testCompareFlatpakConfigsWhereNumberOfGroupsDiffers()
    {
        KConfig configA(QString(), KConfig::SimpleConfig);
        KConfig configB(QString(), KConfig::SimpleConfig);

        const std::array configs = {&configA, &configB};
        for (const auto &config : configs) {
            const std::array groupNames = {QLatin1String("G1"), QLatin1String("G2")};
            for (const auto &groupName : groupNames) {
                auto group = config->group(groupName);
                group.writeEntry(QStringLiteral("key1"), QStringLiteral("value1"));
            }
        }
        QVERIFY(operatorFlatpakConfigEquals(configA, configB));
        const auto extraGroupName = QLatin1String("GExtra");
        {
            auto groupA3 = configA.group(extraGroupName);
            groupA3.writeEntry(QStringLiteral("key1"), QStringLiteral("value1"));
        }
        QVERIFY(!operatorFlatpakConfigEquals(configA, configB));
        configA.deleteGroup(extraGroupName);
        QVERIFY(operatorFlatpakConfigEquals(configA, configB));
        {
            // Test the other way around too
            auto groupB3 = configB.group(extraGroupName);
            groupB3.writeEntry(QStringLiteral("key1"), QStringLiteral("value1"));
        }
        QVERIFY(!operatorFlatpakConfigEquals(configA, configB));
        configB.deleteGroup(extraGroupName);
        QVERIFY(operatorFlatpakConfigEquals(configA, configB));
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
            const auto section = model.data(model.index(i, 0), FlatpakPermissionModel::Section).value<FlatpakPermissionsSectionType::Type>();
            if (section == FlatpakPermissionsSectionType::Filesystems) {
                filesystems.append(name);
            }
        }

        // Note: update this name when more standard filesystems are added.
        const QString hostEtc = QLatin1String("host-etc");
        const auto indexOfHostEtc = filesystems.indexOf(hostEtc);
        QCOMPARE(indexOfHostEtc, 3);
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
            const auto section = model.data(model.index(i, 0), FlatpakPermissionModel::Section).value<FlatpakPermissionsSectionType::Type>();
            if (section == FlatpakPermissionsSectionType::SessionBus) {
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
        const auto isDefaultEnabled0 = model.data(model.index(indexOfService0, 0), FlatpakPermissionModel::IsDefaultEnabled).toBool();
        const auto isDefaultEnabled1 = model.data(model.index(indexOfService1, 0), FlatpakPermissionModel::IsDefaultEnabled).toBool();
        const auto isDefaultEnabled2 = model.data(model.index(indexOfService2, 0), FlatpakPermissionModel::IsDefaultEnabled).toBool();
        QVERIFY(isDefaultEnabled0);
        QVERIFY(!isDefaultEnabled1);
        QVERIFY(!isDefaultEnabled2);

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
            const auto values = model.data(model.index(row, 0), FlatpakPermissionModel::ValuesModel).value<QAbstractListModel *>();
            QVERIFY(modelContains(values, i18n("None")));
            QVERIFY(modelContains(values, i18n("see")));
            QVERIFY(modelContains(values, i18n("talk")));
            QVERIFY(modelContains(values, i18n("own")));
        };
        checkPossibleValues(indexOfService0);
        checkPossibleValues(indexOfService1);
        checkPossibleValues(indexOfService2);

        const auto isEffectiveEnabled = [&](int row) -> bool {
            return model.data(model.index(row, 0), FlatpakPermissionModel::IsEffectiveEnabled).toBool();
        };
        QVERIFY(isEffectiveEnabled(indexOfService0));
        QVERIFY(isEffectiveEnabled(indexOfService1));
        QVERIFY(isEffectiveEnabled(indexOfService2));

        // Toggling non-default bus entry should disable it (i.e. mark for deletion)
        model.togglePermissionAtIndex(indexOfService2);
        QVERIFY(!isEffectiveEnabled(indexOfService2));
        // Reloading data should re-enable it again.
        model.load();
        QVERIFY(isEffectiveEnabled(indexOfService2));
        // Disabling non-default bus entry and saving it should remove it from override file.
        model.togglePermissionAtIndex(indexOfService2);
        QVERIFY(!isEffectiveEnabled(indexOfService2));
        model.save();
        {
            const KConfig expectedDesktopFile(QFINDTESTDATA("fixtures/overrides/org.gnome.dfeet"));
            const auto group = expectedDesktopFile.group(QLatin1String(FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY));
            QVERIFY(!group.hasKey(service2));
        }
        // Re-enabling non-default bus entry and saving it should add it back to override file.
        model.togglePermissionAtIndex(indexOfService2);
        QVERIFY(isEffectiveEnabled(indexOfService2));
        model.save();
        {
            const KConfig expectedDesktopFile(QFINDTESTDATA("fixtures/overrides/org.gnome.dfeet"));
            const auto group = expectedDesktopFile.group(QLatin1String(FLATPAK_METADATA_GROUP_SESSION_BUS_POLICY));
            QVERIFY(group.hasKey(service2));
        }
    }

    void testDBusBrokenPolicies()
    {
        FlatpakReferencesModel referencesModel;
        QFile metadataFile(QFINDTESTDATA("fixtures/metadata/org.gnome.Boxes"));
        QVERIFY(metadataFile.open(QFile::ReadOnly));
        FlatpakReference reference(&referencesModel,
                                   "org.gnome.Boxes",
                                   "x86_64",
                                   "stable",
                                   "43.1",
                                   "Boxes",
                                   QFINDTESTDATA("fixtures/overrides/"),
                                   QUrl(),
                                   metadataFile.readAll());
        FlatpakPermissionModel model;
        model.setReference(&reference);
        model.load();
        model.setShowAdvanced(true);

        // This service is set to "hello" by default (in metadata).
        const auto session0 = QLatin1String("org.freedesktop.secrets");
        int indexOfSession0 = -1;
        // This service is set to "talk" by default (in metadata) but overriden with an invalid policy string.
        const auto session1 = QLatin1String("ca.desrt.dconf");
        int indexOfSession1 = -1;
        // This service is set to an invalid policy string by default (in metadata) but overridden with "see".
        const auto system = QLatin1String("org.freedesktop.timedate1");
        int indexOfSystem = -1;

        for (auto i = 0; i <= model.rowCount(); ++i) {
            const auto name = model.data(model.index(i, 0), FlatpakPermissionModel::Name).toString();
            const auto section = model.data(model.index(i, 0), FlatpakPermissionModel::Section).value<FlatpakPermissionsSectionType::Type>();
            if (section == FlatpakPermissionsSectionType::SessionBus) {
                if (name == session0) {
                    indexOfSession0 = i;
                } else if (name == session1) {
                    indexOfSession1 = i;
                }
            } else if (section == FlatpakPermissionsSectionType::SystemBus) {
                if (name == system) {
                    indexOfSystem = i;
                }
            }
        }
        QVERIFY(indexOfSession0 != -1);
        QVERIFY(indexOfSession1 != -1);
        QVERIFY(indexOfSystem != -1);

        QVERIFY(model.data(model.index(indexOfSession0, 0), FlatpakPermissionModel::IsDefaultEnabled).toBool());
        QCOMPARE(model.data(model.index(indexOfSession0, 0), FlatpakPermissionModel::DefaultValue).toString(), i18n("None"));
        QCOMPARE(model.data(model.index(indexOfSession0, 0), FlatpakPermissionModel::EffectiveValue).toString(), i18n("None"));

        QVERIFY(model.data(model.index(indexOfSession1, 0), FlatpakPermissionModel::IsDefaultEnabled).toBool());
        QVERIFY(model.data(model.index(indexOfSession1, 0), FlatpakPermissionModel::IsEffectiveEnabled).toBool());
        QCOMPARE(model.data(model.index(indexOfSession1, 0), FlatpakPermissionModel::DefaultValue).toString(), i18n("talk"));
        QCOMPARE(model.data(model.index(indexOfSession1, 0), FlatpakPermissionModel::EffectiveValue).toString(), i18n("None"));

        QVERIFY(model.data(model.index(indexOfSystem, 0), FlatpakPermissionModel::IsDefaultEnabled).toBool());
        QVERIFY(model.data(model.index(indexOfSystem, 0), FlatpakPermissionModel::IsEffectiveEnabled).toBool());
        QCOMPARE(model.data(model.index(indexOfSystem, 0), FlatpakPermissionModel::DefaultValue).toString(), i18n("None"));
        QCOMPARE(model.data(model.index(indexOfSystem, 0), FlatpakPermissionModel::EffectiveValue).toString(), i18n("see"));
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

        const auto filesystem = QLatin1String("~/path");
        const auto session = QLatin1String("com.example.session");
        const auto system = QLatin1String("com.example.system");
        const auto envName = QLatin1String("SOME_ENV");
        const auto envValue = QLatin1String("abc123");

        model.addUserEnteredPermission(FlatpakPermissionsSectionType::Filesystems, filesystem, i18n("create"));
        model.addUserEnteredPermission(FlatpakPermissionsSectionType::SessionBus, session, i18n("talk"));
        model.addUserEnteredPermission(FlatpakPermissionsSectionType::SystemBus, system, i18n("see"));
        model.addUserEnteredPermission(FlatpakPermissionsSectionType::Environment, envName, envValue);

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

            if (name == filesystem) {
                QVERIFY(!model.data(model.index(i, 0), FlatpakPermissionModel::IsDefaultEnabled).toBool());
                QVERIFY(model.data(model.index(i, 0), FlatpakPermissionModel::IsEffectiveEnabled).toBool());
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::DefaultValue).toString(), i18n("create"));
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::EffectiveValue).toString(), i18n("create"));
            }

            if (name == session) {
                QVERIFY(!model.data(model.index(i, 0), FlatpakPermissionModel::IsDefaultEnabled).toBool());
                QVERIFY(model.data(model.index(i, 0), FlatpakPermissionModel::IsEffectiveEnabled).toBool());
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::DefaultValue).toString(), i18n("talk"));
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::EffectiveValue).toString(), i18n("talk"));
            }

            if (name == system) {
                QVERIFY(!model.data(model.index(i, 0), FlatpakPermissionModel::IsDefaultEnabled).toBool());
                QVERIFY(model.data(model.index(i, 0), FlatpakPermissionModel::IsEffectiveEnabled).toBool());
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::DefaultValue).toString(), i18n("see"));
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::EffectiveValue).toString(), i18n("see"));
            }

            if (name == envName) {
                QVERIFY(!model.data(model.index(i, 0), FlatpakPermissionModel::IsDefaultEnabled).toBool());
                QVERIFY(model.data(model.index(i, 0), FlatpakPermissionModel::IsEffectiveEnabled).toBool());
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::DefaultValue).toString(), envValue);
                QCOMPARE(model.data(model.index(i, 0), FlatpakPermissionModel::EffectiveValue).toString(), envValue);
            }
        }
        model.save();
        const KConfig actual(QFINDTESTDATA("fixtures/overrides/com.discordapp.Discord"));
        const KConfig expected(QFINDTESTDATA("fixtures/overrides.out/com.discordapp.Discord"));
        QVERIFY(operatorFlatpakConfigEquals(actual, expected));
    }

    void testValuesModelForSectionsWithoutModels()
    {
        const auto values = FlatpakPermissionModel::valuesModelForSectionType(FlatpakPermissionsSectionType::Features);
        QCOMPARE(values, nullptr);
    }

    void testValuesModelForFilesystemsSection()
    {
        const auto values = FlatpakPermissionModel::valuesModelForFilesystemsSection();
        QCOMPARE(values->rowCount(QModelIndex()), 3);
        QEXPECT_FAIL("", "Filesystems logic is not ready to use 'OFF' value yet.", Continue);
        QVERIFY(modelContains(values, i18n("OFF"))); // Filesystems logic is not ready to use this value yet.
        QVERIFY(modelContains(values, i18n("read/write")));
        QVERIFY(modelContains(values, i18n("read-only")));
        QVERIFY(modelContains(values, i18n("create")));
    }

    void testValuesModelForBusSections()
    {
        const auto values = FlatpakPermissionModel::valuesModelForBusSections();
        QCOMPARE(values->rowCount(QModelIndex()), 4);
        QVERIFY(modelContains(values, i18n("None")));
        QVERIFY(modelContains(values, i18n("talk")));
        QVERIFY(modelContains(values, i18n("own")));
        QVERIFY(modelContains(values, i18n("see")));
    }

    void testValuesModelMapping()
    {
        auto expected = FlatpakPermissionModel::valuesModelForFilesystemsSection();
        auto actual = FlatpakPermissionModel::valuesModelForSectionType(FlatpakPermissionsSectionType::Filesystems);
        QCOMPARE(actual, expected);

        expected = FlatpakPermissionModel::valuesModelForBusSections();
        actual = FlatpakPermissionModel::valuesModelForSectionType(FlatpakPermissionsSectionType::SessionBus);
        QCOMPARE(actual, expected);

        actual = FlatpakPermissionModel::valuesModelForSectionType(FlatpakPermissionsSectionType::SystemBus);
        QCOMPARE(actual, expected);
    }
};

QTEST_MAIN(FlatpakPermissionModelTest)

#include "flatpakpermissiontest.moc"
