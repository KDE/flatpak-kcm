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

    enum OriginType {
        /**
         * Built-in type is for all pre-defined system resources (permissions)
         * as found in flatpak-metadata(5) man page, and any other additional
         * resources declared in app metadata.
         *
         * They shall not be removed from the list of permissions when
         * unchecked.
         *
         * Predefined resources come with translated description.
         */
        // TODO: Instead of unchecking there should be more obvious UI. For Bus
        // type, there's a "none" value. For environment we should implement
        // "unset-environment" category. For filesystem paths it is not
        // specified, so we probably should remove the ability to "uncheck"
        // them, and show a "Remove" button instead.
        BuiltIn,
        /**
         * User-defined permissions are resources that user has manually added
         * in their overrides. In other words, they are not present in app
         * metadata manifest, and can be removed completely when unchecked.
         */
        // TODO: Same as in BuiltIn, consider "Remove" button instead of unchecking.
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
     * Create a Dummy entry in the Advanced section, just so that ListView shows
     * a section header even if there are no actual rows in it.
     */
    explicit FlatpakPermission(const QString &name, const QString &category);

    explicit FlatpakPermission(const QString &name,
                               const QString &category,
                               const QString &description,
                               ValueType type,
                               bool isDefaultEnabled,
                               const QString &defaultValue = {},
                               const QStringList &possibleValues = {});

    /**
     * Technical untranslated name of the resource managed by this permission entry.
     *
     * See ValueType enum for more.
     */
    const QString &name() const;

    /**
     * Technical untranslated category name of the resource managed by this permission entry.
     *
     * See ValueType enum for more.
     */
    const QString &category() const;

    /**
     * User-facing translated string used as section header.
     */
    QString categoryHeading() const;

    /**
     * Untranslate section heading back into category identifier. It's a hack
     * until the model is refactored to only operate on identifiers, and all
     * i18n stuff is moved elsewhere.
     */
    static QString categoryHeadingToRawCategory(const QString &section);

    /**
     * User-facing translated description of the resource managed by this permission entry.
     *
     * See ValueType enum for more.
     */
    const QString &description() const;

    ValueType valueType() const;

    /**
     * Type of permission this entry represents.
     */
    OriginType originType() const;

    /**
     * Set which type of permissions this entry represents.
     */
    // TODO: This method should be replaced with constructor argument.
    void setOriginType(OriginType type);

    /**
     * Section type of this permission. Defaults to SectionType::Basic for
     * ValueType::Filesystems, and Advanced for everything else.
     */
    SectionType sectionType() const;

    /**
     * Set section type for this permission.
     */
    // TODO: This method should be replaced with constructor argument.
    void setSectionType(SectionType type);

    /**
     * System default "enabled" status of this permission. It can not be modified.
     */
    bool isDefaultEnabled() const;

    /** Set user override */
    void setOverrideEnabled(bool enabled);

    /**
     * This property reports the current effective "enabled" status of this
     * permission in KCM.
     *
     * For ValueType::Simple permissions, if current enabled status matches
     * system defaults, it will be removed from user overrides.
     *
     * For user-defined permissions of other ValueType types, disabling them
     * would mark them for removal.
     */
    // TODO: What should it do for built-in permissions of other ValueType
    // types? It doesn't make much sense.
    bool isEffectiveEnabled() const;
    void setEffectiveEnabled(bool enabled);

    /**
     * System default value for this permission. It can not be modified.
     *
     * Applicable for any permissions other than ValueType::Simple.
     */
    const QString &defaultValue() const;

    // TODO: Remove this method.
    const QString &overrideValue() const;

    /** Set user override */
    void setOverrideValue(const QString &value);

    /**
     * This property holds the current effective value of this permission in
     * KCM.
     *
     * See ValueType enum for more.
     */
    const QString &effectiveValue() const;
    void setEffectiveValue(const QString &value);

    /** Untranslate value of ValueTyep::Filesystems permission. */
    // TODO: Remove this method, store enum variants or otherwise raw untranslated data.
    QString fsCurrentValue() const;

    /**
     * Provide a user-facing translated list of possible values for this kind of
     * permission.
     */
    // TODO: It should be a model that also contains detailed description
    // (help text) and untranslated value identifier.
    const QStringList &possibleValues() const;

    /** Integration with KCM. */
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

    ValueType m_valueType;
    OriginType m_originType;
    SectionType m_sectionType;

    /* Applicable for all ValueType permissions. */

    /** System defaults */
    bool m_defaultEnable;
    /** User overrides */
    bool m_overrideEnable;
    /** Current value in KCM */
    bool m_effectiveEnable;

    /* Applicable for any permissions other than ValueType::Simple. */

    /** System defaults */
    QString m_defaultValue;
    /** User overrides */
    QString m_overrideValue;
    /** Current value in KCM */
    QString m_effectiveValue;

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

    FlatpakReference *reference() const;
    void setReference(FlatpakReference *ref);

    void load();
    void save();
    void defaults();
    bool isDefaults() const;
    bool isSaveNeeded() const;

    Q_INVOKABLE QStringList valueListForSection(const QString &sectionHeader) const;
    Q_INVOKABLE QStringList valueListForUntranslatedCategory(const QString &category) const;

public Q_SLOTS:
    void togglePermissionAtIndex(int index);
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
