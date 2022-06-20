/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>
#include <QAbstractListModel>

class FlatpakPermission
{
public:
    enum ValueType {
        Simple, /* on/off values, eg: internet connection */
        Complex /* other values, eg: talk/own permissions to session buses */
    };

    FlatpakPermission(QString name = QString(), QString category = QString(), QString description = QString(), QString value = QString(), ValueType type = ValueType::Simple);
    QString name() const;
    QString category() const;
    QString description() const;
    QString value() const;
    ValueType type() const;

//    void setValue(QString value);

private:
    QString m_name;
    QString m_category;
    QString m_description;
    QString m_value;
    ValueType m_type;
};

class FlatpakPermissionModel : public QAbstractListModel
{
    Q_OBJECT
public:
    FlatpakPermissionModel(QObject *parent = nullptr);

    enum Roles {
        Name = Qt::UserRole + 1,
        Category,
        Description,
        Value,
        Type
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &parent, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

private:
    QVector<FlatpakPermission> m_permissions;
};
