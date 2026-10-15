// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_LIBRARY_PREFERENCES_MODEL_HPP
#define VLC_QT_LIBRARY_PREFERENCES_MODEL_HPP
#include <QObject>
#include <QVariantMap>
#include <QByteArray>
struct qt_intf_t;
class LibraryPreferencesModel final:public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QVariantMap capabilities READ capabilities NOTIFY capabilitiesChanged FINAL)
    Q_PROPERTY(QVariantMap choices READ choices NOTIFY choicesChanged FINAL)
    Q_PROPERTY(QVariantMap applyModes READ applyModes CONSTANT FINAL)
    Q_PROPERTY(QVariantMap supportNotes READ supportNotes NOTIFY valuesChanged FINAL)
    Q_PROPERTY(bool libraryAvailable READ libraryAvailable NOTIFY valuesChanged FINAL)
    Q_PROPERTY(bool legacyFileManagerOnly READ legacyFileManagerOnly NOTIFY valuesChanged FINAL)
    Q_PROPERTY(int nativeFileManagerPolicy READ nativeFileManagerPolicy NOTIFY valuesChanged FINAL)
    Q_PROPERTY(int nativeOpenMode READ nativeOpenMode NOTIFY valuesChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(bool restartRequired READ restartRequired NOTIFY restartRequiredChanged FINAL)
public:
    struct Consumers {
        bool instancePolicy=false; // Actual Win32 decision consumer; desktop IPC still separate gate.
        bool openingQueue=false; // Actual bounded receiver/typed sender protocol.
        bool metadataFetch=false; // Actual automatic playlist + ML fetch scopes.
        bool hiddenAttributes=false; // Win32 filesystem hidden-attribute consumer.
    };
    explicit LibraryPreferencesModel(qt_intf_t *,QObject *parent=nullptr);
    LibraryPreferencesModel(qt_intf_t *,Consumers,QObject *parent=nullptr);
    QVariantMap values()const{return m_values;}
    QVariantMap capabilities()const{return m_capabilities;}
    QVariantMap choices()const{return m_choices;}
    QVariantMap applyModes()const;
    QVariantMap supportNotes()const;
    bool libraryAvailable()const;
    bool legacyFileManagerOnly()const;
    int nativeFileManagerPolicy()const;
    int nativeOpenMode()const;
    QString lastError()const{return m_error;}
    bool restartRequired()const{return m_restart;}
    Q_INVOKABLE bool setValue(const QString &,const QVariant &);
    Q_INVOKABLE void reload();
signals:
    void valuesChanged();
    void capabilitiesChanged();
    void choicesChanged();
    void lastErrorChanged();
    void restartRequiredChanged();
private:
    void refresh();
    QByteArray fingerprint()const;
    void setError(const QString &);
    bool fail(const QString &);
    qt_intf_t *m_intf;
    Consumers m_consumers;
    QVariantMap m_values,m_capabilities,m_choices;
    QByteArray m_startup;
    QString m_error;
    bool m_restart=false,m_lastLibraryAvailable=false,m_lastLegacyFileOnly=false;
    int m_lastFilePolicy=-1,m_lastOpenMode=-1;
};
#endif
