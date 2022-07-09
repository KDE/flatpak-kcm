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
        Complex /* other values, eg: talk/own permissions to session buses */
    };

    FlatpakPermission(QString name = QString(), QString category = QString(), QString description = QString(), QString defaultValue = QStringLiteral("OFF"), QStringList possibleValues = QStringList(), QString currentValue = QString(), ValueType type = ValueType::Simple);
    QString name() const;
    QString category() const;
    QString description() const;
    QString defaultValue() const;
    QStringList possibleValues() const;
    QString currentValue() const;
    ValueType type() const;

    void setCurrentValue(QString val);

private:
    QString m_name;
    QString m_category;
    QString m_description;
    QString m_defaultValue;
    QStringList m_possibleValues;
    QString m_currentValue;
    ValueType m_type;
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
        IsComplex,
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

Q_SIGNALS:
    void referenceChanged();

private:
    QVector<FlatpakPermission> m_permissions;
    FlatpakReference *m_reference;
};
