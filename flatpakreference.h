/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>
#include <QVector>
#include <QAbstractListModel>

class FlatpakReference
{
public:
    FlatpakReference(QString name, QString id, QString version, QString icon = QString());
    QString name() const;
    QString version() const;
    QString id() const;
    QString icon() const;

private:
    QString m_name;
    QString m_version;
    QString m_id;
    QString m_icon;
};

class FlatpakReferencesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    FlatpakReferencesModel();

    enum Roles {
        Name = Qt::UserRole + 1,
        Id,
        Version,
        Icon
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &parent, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

private:
    QVector<FlatpakReference> m_references;
};
