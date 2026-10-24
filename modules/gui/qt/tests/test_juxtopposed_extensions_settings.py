# SPDX-License-Identifier: GPL-2.0-or-later
"""Load the actual Extensions QML with Qt, including semantic property checks.
This is a startup regression guard, not native consumer or fidelity acceptance.
"""
from pathlib import Path
import argparse,hashlib,json,os
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-dir',type=Path,required=True)
args=parser.parse_args();output=args.output_dir.resolve();output.mkdir(parents=True,exist_ok=False)
root=Path(__file__).resolve().parents[4]
os.environ.update(QT_QPA_PLATFORM='offscreen',QT_QUICK_BACKEND='software',QML_DISABLE_DISK_CACHE='1',QT_QPA_FONTDIR=str(root/'modules/gui/qt/pixmaps/juxtopposed'))
from PySide6.QtCore import QObject,QUrl,QCoreApplication,QEvent,qInstallMessageHandler
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlEngine,QQmlComponent
messages=[];qInstallMessageHandler(lambda kind,context,message:messages.append(message))
app=QGuiApplication([]);engine=QQmlEngine()
page=root/'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedExtensionsSettings.qml'
component=QQmlComponent(engine,QUrl.fromLocalFile(str(page)))
assert component.isReady(),[error.toString() for error in component.errors()]
item=component.createWithInitialProperties({'preferences':None,'assetBase':QUrl.fromLocalFile(str(root/'modules/gui/qt/pixmaps/juxtopposed')+'/'),'width':790})
assert item is not None,[error.toString() for error in component.errors()]
assert item.objectName()=='juxtopposedExtensionsSettings'
assert item.findChild(QObject,'extensionsList') is not None
item.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete)
component.deleteLater();engine.deleteLater();QCoreApplication.sendPostedEvents(None,QEvent.DeferredDelete)
errors=[message for message in messages if any(term in message for term in ['ReferenceError','TypeError','is not a type','Cannot assign','Unable to assign','Required property','Cannot load'])]
assert not errors,errors
(output/'verification.json').write_text(json.dumps(dict(passed=True,checks=['Actual page and shared dependencies compile with Qt semantic property validation','Actual page instantiates with required properties and its list'],sourceSha256=hashlib.sha256(page.read_bytes()).hexdigest(),qmlMessages=messages,nativeConsumersAccepted=False,visualFidelityAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print('PASS: 2 actual Extensions QML loading checks')
