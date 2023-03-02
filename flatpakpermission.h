/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "flatpakreference.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QString>

class FlatpakReference;

/**
 * @class FlatpakPermission describes a single configurable entry in the list model of permissions.
 *
 * The content of instances of this class can be interpretted in different ways depending on their
 * value type(), permission pType() and section sType().
 *
 * See flatpak-metadata(5) for more.
 */
class FlatpakPermission
{
public:
    enum ValueType {
        /**
         * This type is for permission entries representing simple boolean
         * toggles.
         *
         * Name of such entry is one of the predefined resource names, e.g.:
         * "bluetooth" from "features" category, "kvm" from "devices" category
         * etc, "pulseaudio" from "sockets" category etc.
         */
        Simple,
        /**
         * Filesystem permissions fall into the "Basic" section type, i.e.
         * always shown.
         *
         * Name of such entry is an actual filesystem path, and the value is
         * one of the suffixes: ":ro", ":rw" (default), ":create".
         */
        Filesystems,
        /**
         * Name of such permission entry is a D-Bus bus name or prefix thereof,
         * for example org.gnome.SessionManager or org.freedesktop.portal.*
         *
         * The possible values for entry are: "none", "see", "talk" or "own".
         */
        Bus,
        /**
         * Name and value of such permission entry are name and value of an
         * environment variable.
         */
        Environment
    };

    enum PermType {
        /**
         * Built-in type is for all pre-defined system resources (permissions)
         * as found in flatpak-metadata(5) man page, and any other additional
         * resources declared in app metadata.
         *
         * They shall not be removed from the list of permissions when
         * unchecked.
         *
         * TODO: Instead of unchecking there should be more obvious UI. For Bus
         * type, there's a "none" value. For environment we should implement
         * "unset-environment" category. For filesystem paths it is not
         * specified, so we probably should remove the ability to "uncheck"
         * them, and show a "Remove" button instead.
         *
         * Predefined resources come with translated description.
         */
        BuiltIn,
        /**
         * User-defined permissions are resources that user has manually added
         * in their overrides. In other words, they are not present in app
         * metadata manifest, and can be removed completely when unchecked.
         *
         * TODO: Same as in BuiltIn, consider "Remove" button instead of unchecking.
         */
        UserDefined,
        /**
         * Empty permissions, just for showing section headers for categories
         * that don't have permissions.
         */
        Dummy
    };

    enum SectionType {
        /**
         * Easy-to-understand permissions, such as: print system access,
         * internet connection, all filesystem resources etc.
         */
        Basic,
        /**
         * More "technical" permissions.
         */
        Advanced
    };

    /**
     * Create dummy section content, just so that ListView shows a section
     * header even if there are no actual rows in it.
     */
    explicit FlatpakPermission(const QString &name, const QString &category);

    FlatpakPermission(const QString &name,
                      const QString &category,
                      const QString &description,
                      ValueType type,
                      bool isEnabledByDefault,
                      const QString &defaultValue = {},
                      const QStringList &possibleValues = {});
    QString name() const;
    QString category() const;
    QString categoryHeading() const;
    /**
     * Untranslate section heading back into category identifier. It's a hack
     * until the model is refactored to only operate on identifiers, and all
     * i18n stuff is moved elsewhere.
     */
    static QString categoryHeadingToRawCategory(const QString &section);
    QString description() const;
    ValueType type() const;
    PermType pType() const;
    SectionType sType() const;

    bool enabled() const;
    bool enabledByDefault() const;

    QString defaultValue() const;
    QStringList possibleValues() const;
    QString currentValue() const;
    QString loadValue() const;

    QString fsCurrentValue() const;

    void setCurrentValue(const QString &val);
    void setLoadValue(const QString &loadValue);
    void setEnabled(bool isEnabled);
    void setLoadEnabled(bool isLoadEnabled);
    void setPType(PermType pType);
    void setSType(SectionType sType);

    bool isSaveNeeded() const;
    bool isDefaults() const;

private:
    /**
     * Untranslatable identifier of permission.
     *
     * For ValueType::Simple permissions, it's the name of the entry in the list of togglable options in that category.
     * For ValueType::Filesystems it's either one of the pre-defined symbolic names or the absolute filepath.
     * For ValueType::Bus permissions it's the the name or glob pattern of D-Bus service(s).
     * For ValueType::Environment permissions it's the name of environment variable.
     */
    QString m_name;
    /** Untranslatable name of [Category] as seen in metadata and override ini-style files. */
    QString m_category;
    /** Human-readable description of the permission, or whatever to be displayed in UI. */
    QString m_description;

    /* Attempts to classify permissions into various types and groups. */

    ValueType m_type;
    PermType m_pType;
    SectionType m_sType;

    /* Applicable for all ValueType permissions. */

    /** System defaults */
    bool m_isEnabledByDefault;
    /** User overrides */
    bool m_isLoadEnabled;
    /** Current value in KCM */
    bool m_isEnabled;

    /* Applicable for any permissions other than ValueType::Simple. */

    /** System defaults */
    QString m_defaultValue;
    /** User overrides */
    QString m_loadValue;
    /** Current value in KCM */
    QString m_currentValue;

    /* Applicable for ValueType::Filesystems and ValueType::Bus only. */

    /** Static list of translated policy names. */
    QStringList m_possibleValues;
};

class FlatpakPermissionModel : public QAbstractListModel
{
    friend class FlatpakPermissionModelTest;
    Q_OBJECT
    Q_PROPERTY(FlatpakReference *reference READ reference WRITE setReference NOTIFY referenceChanged)
public:
    using QAbstractListModel::QAbstractListModel;

    enum Roles {
        Name = Qt::UserRole + 1,
        Category,
        Description,
        ValueList,
        CurrentValue,
        DefaultValue,
        IsGranted,
        Type,
        IsSimple,
        IsEnvironment,
        Path,
        IsNotDummy,
        SectionType,
        IsBasic
    };
    Q_ENUM(Roles)

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void loadDefaultValues();
    void loadCurrentValues();

    FlatpakReference *reference();

    void load();
    void save();
    void defaults();
    bool isDefaults() const;
    bool isSaveNeeded() const;

    Q_INVOKABLE QStringList valueListForSection(const QString &sectionHeader) const;
    Q_INVOKABLE QStringList valueListForUntranslatedCategory(const QString &category) const;

public Q_SLOTS:
    void setReference(FlatpakReference *ref);
    void setPerm(int index);
    void editPerm(int index, const QString &newValue);
    void addUserEnteredPermission(const QString &name, QString cat, const QString &value);

Q_SIGNALS:
    void referenceChanged();

private:
    void addPermission(FlatpakPermission *perm, bool shouldBeOn);
    void removePermission(FlatpakPermission *perm, bool isGranted);
    void addBusPermissions(FlatpakPermission *perm);
    void removeBusPermission(FlatpakPermission *perm);
    void editFilesystemsPermissions(FlatpakPermission *perm, const QString &newValue);
    void editBusPermissions(FlatpakPermission *perm, const QString &newValue);
    void addEnvPermission(FlatpakPermission *perm);
    void removeEnvPermission(FlatpakPermission *perm);
    void editEnvPermission(FlatpakPermission *perm, const QString &newValue);
    bool permExists(const QString &name);
    int permIndex(const QString &category);
    void readFromFile();
    void writeToFile();

    QVector<FlatpakPermission> m_permissions;
    QPointer<FlatpakReference> m_reference;
    QString m_overridesData;
};
