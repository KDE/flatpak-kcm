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
class FlatpakPermissionModel;

class FlatpakReference : public QObject
{
    Q_OBJECT
public:
    ~FlatpakReference() override;
    explicit FlatpakReference(FlatpakReferencesModel *parent, QString name, QString id, const QString &path, QString version, QString icon = QString(), QByteArray metadata = QByteArray(), FlatpakReferencesModel *refsModel = nullptr);
    QString name() const;
    QString displayName() const;
    QString version() const;
    QString icon() const;
    QString path() const;
    QByteArray metadata() const;

    FlatpakPermissionModel *permsModel();
    void setPermsModel(FlatpakPermissionModel *permsModel);

    void load();
    void save();
    void defaults();
    bool isSaveNeeded() const;
    bool isDefaults() const;

Q_SIGNALS:
    void needsLoad();
    void needsSaveChanged();

private:
    QString m_name;
    QString m_id;
    QString m_version;
    QString m_icon;
    QString m_path;
    QByteArray m_metadata;

    FlatpakPermissionModel *m_permsModel;
    FlatpakReferencesModel *m_refsModel;
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
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void load(int index);
    void save(int index);
    void defaults(int index);
    bool isSaveNeeded(int index) const;
    bool isDefaults(int index) const;

Q_SIGNALS:
    void needsLoad();
    void needsSaveChanged();

private:
    QVector<FlatpakReference*> m_references;
};
