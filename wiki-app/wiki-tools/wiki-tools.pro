TEMPLATE = app
TARGET   = wiki-tools

INCLUDEPATH += src/ mozilla-sha1/

HEADERS += \
    mozilla-sha1/sha1.h
SOURCES += \
    mozilla-sha1/sha1.c

# Core functionality
HEADERS += \
    src/ArticleHandler.h \
    src/Article.h \
    src/Compression.h \
    src/StreamReader.h \
    src/Title.h

SOURCES += \
    src/ArticleHandler.cc \
    src/Article.cc \
    src/Compression.cc \
    src/StreamReader.cc \
    src/Title.cc

LIBS += -lz -lbz2
    

# Extractors/Functionality
HEADERS += \
    src/CreateIndex.h \
    src/CreateText.h \
    src/ExtractArticleUrl.h \
    src/ExtractWords.h \
    src/ExtractText.h \
    src/ExtractTextCompressed.h \
    src/ExtractTextHashed.h \
    src/ExtractTitles.h \
    src/SplitArticles.h
SOURCES += \
    src/CreateIndex.cc \
    src/CreateText.cc \
    src/ExtractArticleUrl.cc \
    src/ExtractWords.cc \
    src/ExtractText.cc \
    src/ExtractTextCompressed.cc \
    src/ExtractTextHashed.cc \
    src/ExtractTitles.cc \
    src/SplitArticles.cc \
    src/main.cc

QT += sql