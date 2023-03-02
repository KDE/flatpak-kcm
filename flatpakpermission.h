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

class FlatpakPermission
{
public:
    enum ValueType {
        Simple, /* on/off values, eg: internet connection */
        Filesystems, /* OFF, read-only, read/write, create */
        Bus,
        Environment
    };

    enum PermType {
        BuiltIn, /* all simple permissions, and all FS, Bus and Env ones that come in metadata */
        UserDefined, /* FS, Bus and Env ones that the user types */
        Dummy /* empty permissions, just for showing section headers for categories that don't have permissions */
    };

    enum SectionType {
        Basic, /* easy-to-understand permissions, includes: print system access, internet connection, filesystems etc. */
        Advanced /* more "technical" permissions */
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
    /** Untranslate section heading back into category identifier.
     * It's a hack until the model is refactored to only operate on identifiers,
     * and all i18n stuff is moved elsewhere.
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
    ValueType m_type;
    PermType m_pType;
    SectionType m_sType;

    /* applicable for all permission types */
    bool m_isEnabled;
    bool m_isEnabledByDefault;
    bool m_isLoadEnabled; /* is it enabled before the user makes any changes in THIS session? */

    /* for non-simple types only */
    QString m_defaultValue;
    QStringList m_possibleValues;
    QString m_currentValue;
    QString m_loadValue; /* what the value was before user made any changes in THIS session */
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
