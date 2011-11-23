#ifndef ENGINE_ENGINE_H
#define ENGINE_ENGINE_H

#include <QDeclarativeListProperty>

namespace engine {

class EnginePrivate ;

class Engine : public QObject {
    Q_OBJECT
    
signals :
    void updated() ;

public :
    Engine( QObject* parent = 0 ) ;
    ~Engine() ;

public slots :
    void load( const QString& path ) ;
    void appendCode( QChar code ) ;
    void appendCode( const QString& code ) ;
    void appendCode( int keycode ) ;
    void popCode() ;
    void reset() ; 
    void select( int index ) ;
    void deselect() ;
    QString getCandidateString() ;
private :
    Q_DISABLE_COPY( Engine ) ;
    Q_DECLARE_PRIVATE( Engine ) ;
    EnginePrivate* d_ptr ;
} ;

}

#endif // ENGINE_ENGINE_H
