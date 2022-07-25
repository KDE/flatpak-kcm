/**
 * SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "flatpakpermission.h"

#include <QString>
#include <QVector>
#include <QAbstractListModel>
class FlatpakReferencesModel;
class FlatpakReference : public QObject
{
    Q_OBJECT
public:
    ~FlatpakReference() override;
    explicit FlatpakReference(FlatpakReferencesModel *parent, QString name, QString m_id, QString version, QString icon = QString(), QByteArray metadata = QByteArray());
    QString name() const;
    QString displayName() const;
    QString version() const;
    QString icon() const;
    QString path() const;
    QByteArray metadata() const;

private:
    QString m_name;
    QString m_id;
    QString m_version;
    QString m_icon;
    QString m_path;
    QByteArray m_metadata;
};

class FlatpakReferencesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit FlatpakReferencesModel(QObject *parent = nullptr);

    enum Roles {
        Name = Qt::UserRole + 1,
        Version,
        Icon,
        Ref
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &parent, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

private:
    QVector<FlatpakReference*> m_references;
};
