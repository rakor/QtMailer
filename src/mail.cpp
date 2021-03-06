/*-
 * Copyright (c) 2015, Martin Kropfinger
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mail.h"

/**
  * @class Mail
  *
  * @brief Represents a mail with all content.
  *
  * The class represents a mail to use with Mailer. It can handle different
  * lists for To, Cc and Bcc and can have multiple attachments if needed.
  */

Mail::Mail(const QStringList &toRecepients, const QStringList &ccRecepients,
           const QStringList &bccRecepients, const QString &sender,
           const QString &subject, const QString &body,
           const QList<QFileInfo> &attachments, QObject *parent) :
    QObject(parent), toRecepients{toRecepients}, ccRecepients{ccRecepients},
    bccRecepients{bccRecepients}, sender{sender}, subject{subject}, body{body},
    attachments{attachments}
{
}

Mail::Mail(const QStringList& toRecepients, const QStringList& ccRecepients,
           const QStringList& bccRecepients, const QString& sender, const QString& subject,
           const QString& body, const QFileInfo& attachment, QObject *parent) :
      Mail(toRecepients, ccRecepients, bccRecepients, sender, subject, body,
           QList<QFileInfo>(), parent)
{
    attachments.append(attachment);
}

Mail::Mail(const QStringList &toRecepients, const QStringList &ccRecepients,
           const QStringList &bccRecepients, const QString &sender, const QString &subject,
           const QString &body, QObject *parent) :
      Mail(toRecepients, ccRecepients, bccRecepients, sender, subject, body,
           QList<QFileInfo>(), parent)
{
}

Mail::Mail(const QStringList &toRecepients, const QString &sender, const QString &subject,
           const QString &body, const QList<QFileInfo> &attachments, QObject *parent) :
      Mail(toRecepients, QStringList(), QStringList(), sender, subject, body, attachments, parent)
{
}

Mail::Mail(const QStringList &toRecepients, const QString &sender, const QString &subject,
           const QString &body, const QFileInfo &attachment, QObject *parent) :
      Mail(toRecepients, QStringList(), QStringList(), sender, subject, body,
           QList<QFileInfo>(), parent)
{
    attachments.append(attachment);
}

Mail::Mail(const QStringList& toRecepients, const QString& sender, const QString& subject,
           const QString& body, QObject *parent) :
      Mail(toRecepients, QStringList(), QStringList(), sender, subject, body,
           QList<QFileInfo>(), parent)
{
}

Mail::Mail(const QString& toRecepient, const QString& sender, const QString& subject,
           const QString& body, QObject *parent) :
      Mail(QStringList(), QStringList(), QStringList(), sender, subject, body,
           QList<QFileInfo>(), parent)
{
    toRecepients.append(toRecepient);
}

Mail::Mail(const QString& toRecepient, const QString& sender, const QString& subject,
           const QString& body, const QFileInfo& attachment, QObject *parent) :
      Mail(QStringList(), QStringList(), QStringList(), sender, subject, body,
           QList<QFileInfo>(), parent)
{
    toRecepients.append(toRecepient);
    attachments.append(attachment);
}

Mail::Mail(const Mail &other) :
    QObject(other.parent()), toRecepients{other.toRecepients}, ccRecepients{other.ccRecepients},
    bccRecepients{other.bccRecepients}, sender{other.sender}, subject{other.subject},
    body{other.body}, attachments{other.attachments}
{
}


/**
 * @brief Takes all information and create one big string representing all the maildata.
 *
 * This string can be given to an smtp-server after DATA. All lines should be folded as needed
 * according to rfc5321. ^^
 *
 * @return String holding the raw maildata
 */
QString Mail::plaintextMail() const
{
    QString message;

    // To- Cc- and Bcc-lines
    if (!toRecepients.isEmpty())
        message.append(recepientHeaderLineFromStringList("To: ", toRecepients));
    if (!ccRecepients.isEmpty())
        message.append(recepientHeaderLineFromStringList("Cc: ", ccRecepients));
    if (!bccRecepients.isEmpty())
        message.append(recepientHeaderLineFromStringList("Bcc: ", bccRecepients));

    // Set From:-line
    message.append("From: "+sender+"\r\n");
    message.append("Subject: "+ subject +"\r\n"); // folding seems to insert whitespaces?! So omitted

    // A multipart message is generated when we have attachments
    if (!attachments.isEmpty()){
        message.append("MIME-Version: 1.0\r\n");
        message.append("Content-type: multipart/mixed; boundary=\"" BOUNDARY "\"\r\n\r\n");
        message.append("--" BOUNDARY "\r\n");
    }

    // Messagebody
    message.append("\r\n"+body+"\r\n");

    // If we have attachments .... add them
    if (!attachments.isEmpty()){
        message.append("--" BOUNDARY "\r\n");
        for (int i{0}; i < attachments.size(); i++){
            message.append("Content-type: "+mimetypeForFile(attachments.at(i))+
                           "; name="+attachments.at(i).fileName()+"\r\n");
            message.append("Content-Transfer-Encoding: base64\r\n");
            message.append("Content-Disposition: attachment; filename="+
                           attachments.at(i).fileName()+"\r\n\r\n");
            message.append(generateBase64FromFile(attachments.at(i)));
            message.append("\r\n--" BOUNDARY);
            if(i == attachments.size()-1) message.append("--");
            message.append("\r\n");

        }
    }

    //clean the string to fit rfc5321
    message.replace( QString::fromLatin1( "\r\n." ), QString::fromLatin1( "\r\n.." ) );
    if (message.right(2) != "\r\n") message.append("\r\n");
    message.append(".\r\n");

    return message;
}


/**
 * Returns the mailsadress of the mailsender
 * @return senderaddress
 */
QString Mail::getSender() const
{
    return sender;
}


/**
 * Returns a QStringList holding all the mailadresses from to, cc and bcc
 * @return all recepients mailaddresses
 */
QStringList Mail::getAllRecepients() const
{
    QStringList result;
    result = toRecepients + ccRecepients + bccRecepients;
    return result;
}


/**
 * All addresses from the To:-header
 * @return recepients
 */
QStringList Mail::getToRecepients() const
{
    return toRecepients;
}


/**
 * All addresses from the Cc:-header
 * @return recepients
 */
QStringList Mail::getCcRecepients() const
{
    return ccRecepients;
}


/**
 * All addresses from the Bcc:-header
 * @return recepients
 */
QStringList Mail::getBccRecepients() const
{
    return bccRecepients;
}


/**
 * Generates a string repesentation in base64 of a file.
 *
 * @param fileinfo  fileinfo pointing to the file to transform
 * @return Stringrepresentation of the file
 */
QString Mail::generateBase64FromFile(const QFileInfo& fileinfo) const
{
    QString attachedFileInBase64;
    if (! fileinfo.exists()) return QString();

    QFile file(fileinfo.absoluteFilePath());
    file.open(QFile::ReadOnly);
    attachedFileInBase64 = QString::fromLatin1((file.readAll().toBase64()).data());
    qint64 i{0};
    while (i < attachedFileInBase64.count()){
        attachedFileInBase64.insert(i, "\r\n");
        i = i+MAXLINESIZE;
    }
    file.close();
    return attachedFileInBase64;
}


/**
 * Takes a string and folds is according rfc5322
 * @param original original string
 * @return folded string
 */
QString Mail::foldString(const QString &original) const
{
    QString foldedString;
    int lineLength{0};
    for (int i{0}; i < original.size(); i++){
        if (lineLength+1 > MAXLINESIZE){
            foldedString.append("\r\n ");
            lineLength = 1;
        }
        if (original.at(i) == '\n' && ( (i < 1) || (original.at(i-1) != '\r') ) ){
                foldedString.append("\r\n");
                lineLength = 0;
        } else{
            foldedString.append(original.at(i));
            lineLength++;
        }
    }
    return foldedString;
}


/**
 * MimeType for a file
 * @param fileinfo pointing to the file
 * @return mimetype of file
 */
QString Mail::mimetypeForFile(const QFileInfo &fileinfo) const
{
    if (!fileinfo.exists()) return QString();
    QMimeDatabase mimedatabase;
    return mimedatabase.mimeTypeForFile(fileinfo).name();
}


/**
 * Takes a QStringList with mailaddresses and generates a folded header line out of them
 * @param header    Header to put in front of the adresses (eg. "To: ", "Cc: ", "Bcc: ")
 * @param addresses QStringList holdig the mailaddresses
 * @return          folded headerline
 */
QString Mail::recepientHeaderLineFromStringList(const QString &header, const QStringList &addresses) const
{
    QString result = header;
    uint linelength = result.size();
    for (int i{0}; i< addresses.count(); i++){ //folding
        if ((linelength + addresses.at(i).size()) >= MAXLINESIZE){
            result.append("\r\n ");
            linelength = 1;
        }
        result.append(addresses.at(i));
        linelength += addresses.at(i).size();
        if (addresses.size() != i+1){
            result.append(",");
            linelength++;
        }
    }
    result.append("\r\n");
    return result;
}
