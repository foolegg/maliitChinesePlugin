#ifndef PINYIN_H
#define PINYIN_H

#include "lookup.h"

#include "split.h"
#include "fit.h"
#include "pick.h"

//#include <QDebug>

namespace lookup {

class PinyinLookup : public Lookup {
public :
    split::KeySet keySet ;
    split::KeyList keyList ;
    fit::KeyMap keyMap ;
    QList<pick::PickPair> pickCache ;
    QList<LookupPair> lookupCache ;
    QList< QList<QString> > preeditCache ;
    QSet<QString> usedKeySet ;
    int candCacheIndex ;
    int candStartIndex ;
    QList<Candidate> candList ;
    int candLength ;

    PinyinLookup( dict::Dictionary* dict ) : Lookup(dict), keySet(), keyList(), keyMap(), pickCache(), lookupCache(), preeditCache(), usedKeySet(), candCacheIndex(0), candStartIndex(0), candList(), candLength(0) {
        this->keyList.append( split::KeyString( QStringList( QString() ), QPair<int, int>() ) ) ;
    }

    virtual void appendCode( QChar code ) {
        this->pickCache.clear() ;
        this->usedKeySet.clear() ;
        this->lookupCache.append( LookupPair() ) ;
        this->preeditCache.append( QList<QString>() ) ;

        split::append_code( &(this->keyList), code, &(this->keySet), this->code.length() ) ;
        this->code.append( code ) ;

        QList<const QString*>* key = &(this->lookupCache.last().second.first)  ;
        QList<const QString*>* preedit = &(this->lookupCache.last().second.second)  ;
        QList<QString>* preeditCache = &(this->preeditCache.last())  ;
        
        int highestPoint = -0x1000 ;
        for ( int i = 0, l = this->keyList.length() ; i < l ; i++ ) {
            const split::KeyString* string = &(this->keyList[i]) ;
            if ( string->second.first == this->code.length() ) {
                int fitPoint ;
                QList<const QString*> buffer ;
                fit::fit( &(string->first), &buffer, &fitPoint, &(this->keyMap) ) ;
                if ( fitPoint > highestPoint ) {
                    highestPoint = fitPoint ;
                    preeditCache->clear() ;
                    key->clear() ;
                    preedit->clear() ;
                    preeditCache->append( string->first.join( QChar( '\'' ) ) ) ;
                    foreach( const QString* s, buffer ) {
                        key->append( s ) ;
                        preedit->append( &(preeditCache->last()) ) ;
                    }
                }
                else if ( fitPoint == highestPoint ) {
                    preeditCache->append( string->first.join( QChar( '\'' ) ) ) ;
                    foreach( const QString* s, buffer ) {
                        key->append( s ) ;
                        preedit->append( &(preeditCache->last()) ) ;
                    }
                }
            }
        }
        //foreach( const QString* k, *key )
            //qDebug() << *k ;
        this->lookupCache.last().first = highestPoint ;
        pick::set( &(this->pickCache), key, preedit, &(this->dict->hash) ) ;
        foreach( const QString* k, *key )
            this->usedKeySet.insert( *k ) ;
        //this->candList.clear() ;
        this->candLength = 0 ;
        this->candCacheIndex = this->lookupCache.length() - 1 ;
        this->candStartIndex = 0 ;
    }

    virtual void popCode() {
        this->pickCache.clear() ;
        this->usedKeySet.clear() ;
        this->lookupCache.removeLast() ;
        this->preeditCache.removeLast() ;

        this->code.chop( 1 ) ;
        split::pop_code( &(this->keyList), this->code.length() ) ;

        if ( !this->lookupCache.isEmpty() ) {
            QList<const QString*>* key = &(this->lookupCache.last().second.first)  ;
            QList<const QString*>* preedit = &(this->lookupCache.last().second.second)  ;
            pick::set( &(this->pickCache), key, preedit, &(this->dict->hash) ) ;
            foreach( const QString* k, *key )
                this->usedKeySet.insert( *k ) ;
            //this->candList.clear() ;
            this->candLength = 0 ;
            this->candCacheIndex = this->lookupCache.length() - 1 ;
            this->candStartIndex = 0 ;
        }
    }

    virtual void clearCode() {
        this->code.clear() ;
        this->pickCache.clear() ;
        this->usedKeySet.clear() ;
        split::clear_code( &this->keyList ) ;
        this->lookupCache.clear() ;
        this->preeditCache.clear() ;
        this->candCacheIndex = 0 ;
        this->candStartIndex = 0 ;
        this->candLength = 0 ;
    }
    
    virtual void setCode( const QString& code ) {
        this->clearCode() ;
        for( int i = 0 ; i < code.length() ; i++ )
            this->appendCode( code.at(i) ) ;
    }
    
    //inline void appendCode( const QString& code ) {
        //for( int i = 0 ; i < code.length() ; i++ )
            //this->appendCode( code.at(i) ) ;
    //}
    
    inline bool checkCache() {
        bool flag = false ;
        if ( this->candCacheIndex >= 0 ) {
            this->pickCache.clear() ;
            QList<const QString*>* key ;
            QList<const QString*>* preedit ;
            int prevPoint = this->lookupCache[this->candCacheIndex].first ;
            int fitPoint ;

            this->candCacheIndex-- ;
            while ( !flag && this->candCacheIndex >= 0 ) {
                fitPoint = this->lookupCache[this->candCacheIndex].first ;
                if ( fitPoint >= prevPoint ) {
                    key = &(this->lookupCache[this->candCacheIndex].second.first)  ;
                    preedit = &(this->lookupCache[this->candCacheIndex].second.second)  ;
                    if ( !key->isEmpty() ) {
                        //qDebug() << fitPoint << prevPoint << *(preedit->at(0)) ;
                        flag = true ;
                    }
                }
                if ( !flag )
                    this->candCacheIndex-- ;
            }
            if ( flag ) {
                this->lookupCache[this->candCacheIndex].first = fitPoint ;
                pick::set( &(this->pickCache), key, preedit, &(this->dict->hash), &(this->usedKeySet) ) ;
                foreach( const QString* k, *key )
                    this->usedKeySet.insert( *k ) ;
            }
        }
        return flag ;
    }

    virtual const Candidate* getCandidate( int index ) {
        //bool flag = !this->code.isEmpty() ;
        bool flag = true ;
        while ( flag && this->candLength <= index ) {
            const QString* key ; const QString* preedit ; const QString* word ; qreal freq ;
            pick::pick( &(this->pickCache), &key, &preedit, &word, &freq ) ;
            if ( key ) {
                //this->candList.append( Candidate( CandPair( KeyPair( key, preedit ), WordPair( word, freq ) ), this->candStartIndex ) ) ;
                check_cand( &(this->candList), this->candLength ) ;
                set_cand( &(this->candList[this->candLength]), key, preedit, word, freq, this->candStartIndex ) ;
                this->candLength++ ;
            }
            else {
                flag = this->checkCache() ;
                //this->candStartIndex = this->candList.length() ;
                this->candStartIndex = this->candLength ;
            }
        }
        if ( flag ) 
            return &(this->candList.at(index)) ; 
        else 
            return NULL ;
    }
};

}

#endif
