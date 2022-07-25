/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "flatpakreference.h"

#include <QString>
#include <QAbstractListModel>

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
        UserDefined /* FS, Bus and Env ones that the user types */
    };

    FlatpakPermission(QString name = QString(), QString category = QString(), QString description = QString(), QString defaultValue = QStringLiteral("OFF"), QStringList possibleValues = QStringList(), QString currentValue = QString(), ValueType type = ValueType::Simple);
    FlatpakPermission(QString name, QString category, QString description, ValueType type, bool isEnabledByDefault, QString defaultValue = QString(), QStringList possibleValues = QStringList());
    QString name() const;
    QString category() const;
    QString categoryHeading() const;
    QString description() const;
    ValueType type() const;
    PermType pType() const;

    bool enabled() const;
    bool enabledByDefault() const;

    QString defaultValue() const;
    QStringList possibleValues() const;
    QString currentValue() const;


    QString fsCurrentValue() const;

    void setCurrentValue(QString val);
    void setEnabled(bool isEnabled);
    void setPType(PermType pType);

private:
    QString m_name;
    QString m_category;
    QString m_description;
    ValueType m_type;
    PermType m_pType;

    /* applicable for all permission types */
    bool m_isEnabled;
    bool m_isEnabledByDefault;

    /* for non-simple types only */
    QString m_defaultValue;
    QStringList m_possibleValues;
    QString m_currentValue;
};

class FlatpakPermissionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(FlatpakReference *reference READ reference WRITE setReference NOTIFY referenceChanged)
public:
    FlatpakPermissionModel(QObject *parent = nullptr);

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
        Path
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &parent, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

    void loadDefaultValues();
    void loadCurrentValues();

    FlatpakReference *reference();

public Q_SLOTS:
    void setReference(FlatpakReference *ref);
    void setPerm(int index, bool isGranted);
    void editPerm(int index, QString newValue);
    void addUserEnteredPermission(QString name, QString cat);

Q_SIGNALS:
    void referenceChanged();

private:
    void addPermission(FlatpakPermission *perm, QString &data, const bool shouldBeOn);
    void removePermission(FlatpakPermission *perm, QString &data, const bool isGranted);
    void addBusPermissions(FlatpakPermission *perm, QString &data);
    void removeBusPermission(FlatpakPermission *perm, QString &data);
    void editFilesystemsPermissions(FlatpakPermission *perm, QString &data, const QString &newValue);
    void editBusPermissions(FlatpakPermission *perm, QString &data, const QString &newValue);
    void addEnvPermission(FlatpakPermission *perm, QString &data);
    void removeEnvPermission(FlatpakPermission *perm, QString &data);
    void editEnvPermission(FlatpakPermission *perm, QString &data, const QString &newValue);
    bool permExists(QString name);
    QVector<FlatpakPermission> m_permissions;
    FlatpakReference *m_reference;
};
