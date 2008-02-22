; #######################################
; czech.nsh
; czech language strings for inkscape installer
; windows code page: 1250
; Authors:
; Michal Kraus Michal.Kraus@wige-mic.cz (original translation)
; Josef Vyb�ral josef.vybiral@gmail.com (update for Inkscape 0.44)
;
; 27 july 2006 new languages en_CA, en_GB, fi, hr, mn, ne, rw, sq
; 11 august 2006 new languages dz bg
; 24 october 2006 new languages en_US@piglatin, th
; 3rd December 2006 new languages eu km
; 14th December 2006 new lng_DeletePrefs, lng_DeletePrefsDesc, lng_WANT_UNINSTALL_BEFORE and lng_OK_CANCEL_DESC
; february 15 2007 new language bn, en_AU, eo, id, ro
; april 11 2007 new language he
; october 2007 new language ca@valencian
; January 2008 new uninstaller messages

!insertmacro MUI_LANGUAGE "Czech"

; Product name
LangString lng_Caption   ${LANG_CZECH} "${PRODUCT_NAME} -- Open Source Editor �k�lovateln� Vektorov� Grafiky(SVG)"

; Button text "Next >" for the license page
LangString lng_LICENSE_BUTTON   ${LANG_CZECH} "Dal�� >"

; Bottom text for the license page
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_CZECH} "$(^Name) je vyd�v�n pod General Public License (GPL). Licen�n� ujedn�n� je zde pouze z informa�n�ch d�vod�. $_CLICK"

;has been installed by different user
LangString lng_DIFFERENT_USER ${LANG_CZECH} "Inkscape byl nainstalov�n u�ivatelem $0.$\r$\nInstalace nemus� b�t dokon�ena spr�vn� pokud v n� budete pokra�ovat!$\r$\nPros�m p�ihlaste se jako $0 a spus�te instalaci znovu."

; want to uninstall before install
LangString lng_WANT_UNINSTALL_BEFORE ${LANG_CZECH} "$R1 je ji� nainstalov�n. $\nChcete p�ed instalac� odstranit p�edchoz� verzi $(^Name) ?"

; press OK to continue press Cancel to abort
LangString lng_OK_CANCEL_DESC ${LANG_CZECH} "$\n$\nStiskn�te OK pro pokra�ov�n�, CANCEL pro p�eru�en�."

;you have no admin rigths
LangString lng_NO_ADMIN ${LANG_CZECH} "Nem�te administr�torsk� opr�vn�n�.$\r$\nInstalace Inkscape pro v�echny u�ivatele nemus� b�t �sp�n� dokon�ena.$\r$\nZru�te ozna�en� volby 'Pro v�echny u�ivatele'."

;win9x is not supported
LangString lng_NOT_SUPPORTED ${LANG_CZECH} "Inkscape neb�� na Windows 95/98/ME!$\r$\nPro podrobn�j�� informace se pros�m obra�te na ofici�ln� webov� str�nky."

; Full install type
LangString lng_Full $(LANG_CZECH) "Pln�"

; Optimal install type
LangString lng_Optimal $(LANG_CZECH) "Optim�ln�"

; Minimal install type
LangString lng_Minimal $(LANG_CZECH) "Minim�ln�"

; Core install section
LangString lng_Core $(LANG_CZECH) "${PRODUCT_NAME} SVG editor (vy�adov�no)"

; Core install section description
LangString lng_CoreDesc $(LANG_CZECH) "Soubory a knihovny ${PRODUCT_NAME}"

; GTK+ install section
LangString lng_GTKFiles $(LANG_CZECH) "GTK+ b�hov� prost�ed� (vy�adov�no)"

; GTK+ install section description
LangString lng_GTKFilesDesc $(LANG_CZECH) "Multiplatformn� sada u�ivatelsk�ho rozhran�, pou�it�ho v ${PRODUCT_NAME}"

; shortcuts install section
LangString lng_Shortcuts $(LANG_CZECH) "Z�stupci"

; shortcuts install section description
LangString lng_ShortcutsDesc $(LANG_CZECH) "Z�stupci pro spu�t�n� ${PRODUCT_NAME}"

; All user install section
LangString lng_Alluser $(LANG_CZECH) "pro v�echny u�ivatele"

; All user install section description
LangString lng_AlluserDesc $(LANG_CZECH) "Instalovat aplikaci pro kohokoliv, kdo pou��v� tento po��ta�.(v�ichni u�ivatel�)"

; Desktop section
LangString lng_Desktop $(LANG_CZECH) "Plocha"

; Desktop section description
LangString lng_DesktopDesc $(LANG_CZECH) "Vytvo�it z�stupce ${PRODUCT_NAME} na plo�e"

; Start Menu  section
LangString lng_Startmenu $(LANG_CZECH) "Nab�dka Start"

; Start Menu section description
LangString lng_StartmenuDesc $(LANG_CZECH) "Vytvo�it pro ${PRODUCT_NAME} polo�ku v nab�dce Start"

; Quick launch section
LangString lng_Quicklaunch $(LANG_CZECH) "Panel rychl�ho spu�t�n�"

; Quick launch section description
LangString lng_QuicklaunchDesc $(LANG_CZECH) "Vytvo�it pro ${PRODUCT_NAME} z�stupce na panelu rychl�ho spu�t�n�"

; File type association for editing
LangString lng_SVGWriter ${LANG_CZECH} "Otv�rat SVG soubory v ${PRODUCT_NAME}"

; File type association for editing description
LangString lng_SVGWriterDesc ${LANG_CZECH} "Vybrat ${PRODUCT_NAME} jako v�choz� editor pro SVG soubory"

; Context Menu
LangString lng_ContextMenu ${LANG_CZECH} "Kontextov� nab�dka"

; Context Menu description
LangString lng_ContextMenuDesc ${LANG_CZECH} "P�idat ${PRODUCT_NAME} do kontextov� nab�dky pro SVG soubory"

; remove personal preferences
LangString lng_DeletePrefs ${LANG_CZECH} "Smazat osobn� nastaven�"

; remove personal preferences description
LangString lng_DeletePrefsDesc ${LANG_CZECH} "Smazat osobn� nastaven� p�edchoz� instalace"



; Additional files section
LangString lng_Addfiles $(LANG_CZECH) "Dal�� soubory"

; Additional files section description
LangString lng_AddfilesDesc $(LANG_CZECH) "Dal�� soubory"

; Examples section
LangString lng_Examples $(LANG_CZECH) "P��klady"

; Examples section description
LangString lng_ExamplesDesc $(LANG_CZECH) "P��klady pou�it� ${PRODUCT_NAME}"

; Tutorials section
LangString lng_Tutorials $(LANG_CZECH) "Pr�vodci"

; Tutorials section description
LangString lng_TutorialsDesc $(LANG_CZECH) "Pr�vodci funkcemi ${PRODUCT_NAME}"


; Languages section
LangString lng_Languages $(LANG_CZECH) "Jazykov� sady"

; Languages section dscription
LangString lng_LanguagesDesc $(LANG_CZECH) "Nainstalovat dal�� jazykov� sady ${PRODUCT_NAME}"

LangString lng_am $(LANG_CZECH) "am  Amharic"
LangString lng_az $(LANG_CZECH) "az  Azerbajd��n�tina"
LangString lng_be $(LANG_CZECH) "be  B�loru�tina"
LangString lng_bg $(LANG_CZECH) "bg  Bulgarian"
LangString lng_bn $(LANG_CZECH) "bn  Bengali"
LangString lng_ca $(LANG_CZECH) "ca  Katal�n�tina"
LangString lng_ca@valencia $(LANG_CZECH) "ca@valencia  Valencian Catalan"
LangString lng_cs $(LANG_CZECH) "cs  �e�tina"
LangString lng_da $(LANG_CZECH) "da  D�n�tina"
LangString lng_de $(LANG_CZECH) "de  N�m�ina"
LangString lng_dz $(LANG_CZECH) "dz  Dzongkha"
LangString lng_el $(LANG_CZECH) "el  �e�tina"
LangString lng_en $(LANG_CZECH) "en  Angli�ina"
LangString lng_en_AU $(LANG_CZECH) "en_AU Australian English"
LangString lng_en_CA $(LANG_CZECH) "en_CA Kanadsk� Angli�tina"
LangString lng_en_GB $(LANG_CZECH) "en_GB Britsk� Angli�tina"
LangString lng_en_US@piglatin $(LANG_CZECH) "en_US@piglatin Pig Latin"
LangString lng_eo $(LANG_CZECH) "eo  Esperanto"
LangString lng_es $(LANG_CZECH) "es  �pan�l�tina"
LangString lng_es_MX $(LANG_CZECH) "es_MX  Mexick� �pan�l�tina"
LangString lng_et $(LANG_CZECH) "et  Eston�tina"
LangString lng_eu $(LANG_CZECH) "eu  Basque"
LangString lng_fi $(LANG_CZECH) "fi  Fin�tina"
LangString lng_fr $(LANG_CZECH) "fr  Francouz�tina"
LangString lng_ga $(LANG_CZECH) "ga  Ir�tina"
LangString lng_gl $(LANG_CZECH) "gl  Gallegan"
LangString lng_he $(LANG_CZECH) "he  Hebrew"
LangString lng_hr $(LANG_CZECH) "hr  Chorvat�tina"
LangString lng_hu $(LANG_CZECH) "hu  Ma�ar�tina"
LangString lng_id $(LANG_CZECH) "id  Indonesian"
LangString lng_it $(LANG_CZECH) "it  Ital�tina"
LangString lng_ja $(LANG_CZECH) "ja  Japon�tina"
LangString lng_km $(LANG_CZECH) "km  Khmer"
LangString lng_ko $(LANG_CZECH) "ko  Korej�tina"
LangString lng_lt $(LANG_CZECH) "lt  Litev�tina"
LangString lng_mk $(LANG_CZECH) "mk  Makedon�tina"
LangString lng_mn $(LANG_CZECH) "mn  Mongol�tina"
LangString lng_ne $(LANG_CZECH) "ne  Nep�l�tina"
LangString lng_nb $(LANG_CZECH) "nb  Nor�tina Bokmal"
LangString lng_nl $(LANG_CZECH) "nl  Holand�tina"
LangString lng_nn $(LANG_CZECH) "nn  Nor�tina Nynorsk"
LangString lng_pa $(LANG_CZECH) "pa  Panjabi"
LangString lng_pl $(LANG_CZECH) "po  Pol�tina"
LangString lng_pt $(LANG_CZECH) "pt  Portugal�tina"
LangString lng_pt_BR $(LANG_CZECH) "pt_BR Brazilsk� Portugal�tina"
LangString lng_ro $(LANG_CZECH) "ro  Romanian"
LangString lng_ru $(LANG_CZECH) "ru  Ru�tina"
LangString lng_rw $(LANG_CZECH) "rw  Kinyarwanda"
LangString lng_sk $(LANG_CZECH) "sk  Sloven�tina"
LangString lng_sl $(LANG_CZECH) "sl  Slovin�tina"
LangString lng_sq $(LANG_CZECH) "sq  Alb�n�tina"
LangString lng_sr $(LANG_CZECH) "sr  Srb�tina"
LangString lng_sr@latin $(LANG_CZECH) "sr@latin  Srb�tina v latince"
LangString lng_sv $(LANG_CZECH) "sv  �v�d�tina"
LangString lng_th $(LANG_CZECH) "th  Thai"
LangString lng_tr $(LANG_CZECH) "tr  Ture�tina"
LangString lng_uk $(LANG_CZECH) "uk  Ukrajin�tina"
LangString lng_vi $(LANG_CZECH) "vi  Vietnam�tina"
LangString lng_zh_CN $(LANG_CZECH) "zh_CH  Zjednodu�en� ��n�tina"
LangString lng_zh_TW $(LANG_CZECH) "zh_TW  Tradi�n� ��n�tina"




; uninstallation options
LangString lng_UInstOpt   ${LANG_CZECH} "Volby pro odinstalov�n�"

; uninstallation options subtitle
LangString lng_UInstOpt1  ${LANG_CZECH} "Vyberte pros�m dal�� nastaven�"

; Ask to purge the personal preferences
LangString lng_PurgePrefs ${LANG_CZECH} "Ponechat osobn� nastaven�"

LangString lng_RETRY_CANCEL_DESC ${LANG_CZECH} "$\n$\nPress RETRY to continue or press CANCEL to abort."

LangString lng_ClearDirectoryBefore ${LANG_CZECH} "${PRODUCT_NAME} must be installed in an empty directory. $INSTDIR is not empty. Please clear this directory first!$(lng_RETRY_CANCEL_DESC)"

LangString lng_UninstallLogNotFound ${LANG_CZECH} "$INSTDIR\uninstall.log not found!$\r$\nPlease uninstall by clearing directory $INSTDIR yourself!"

LangString lng_FileChanged ${LANG_CZECH} "The file $filename has been changed after installation.$\r$\nDo you still want to delete that file?"

LangString lng_Yes ${LANG_CZECH} "Yes"

LangString lng_AlwaysYes ${LANG_CZECH} "always answer Yes"

LangString lng_No ${LANG_CZECH} "No"

LangString lng_AlwaysNo ${LANG_CZECH} "always answer No"
