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
        Environment,
        Complex /* other values, eg: talk/own permissions to session buses */
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

    explicit FlatpakPermission(const QString &name = {},
                               const QString &category = {},
                               const QString &description = {},
                               const QString &defaultValue = QStringLiteral("OFF"),
                               const QStringList &possibleValues = {},
                               const QString &currentValue = {},
                               ValueType type = ValueType::Simple);
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
    QString m_name;
    QString m_category;
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

    Q_INVOKABLE QStringList valueList(const QString &catHeader) const;

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
    int permIndex(const QString &category, int from = 0);
    void readFromFile();
    void writeToFile();

    QVector<FlatpakPermission> m_permissions;
    QPointer<FlatpakReference> m_reference;
    QString m_overridesData;
};
