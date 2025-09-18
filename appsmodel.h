/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2025 David Redondo <kde@david-redondo.de>
*/

#pragma once

#include <KService>

#include <QAbstractListModel>

class AppsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        AppIdRole = Qt::UserRole,
        IsHostRole
    };

    explicit AppsModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QString findIconNameById(const QString &id) const;

private:
    void loadApps();
    QList<KService::Ptr> m_apps;
};
