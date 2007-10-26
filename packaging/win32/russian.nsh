; #######################################
; russian.nsh
; russian language strings for inkscape installer
; windows code page: 1251
; Authors:
; Alexandre Prokoudine alexandre.prokoudine@gmail.com
;
; 27 july 2006 new languages en_CA, en_GB, fi, hr, mn, ne, rw, sq
; 11 august 2006 new languages dz bg
; 24 october 2006 new languages en_US@piglatin, th
; 3rd December 2006 new languages eu km
; 14th December 2006 new lng_DeletePrefs, lng_DeletePrefsDesc, lng_WANT_UNINSTALL_BEFORE and lng_OK_CANCEL_DESC; 2nd February 2007 new language ru
; february 15 2007 new language bn, en_AU, eo, id, ro
; april 11 2007 new language he
; october 2007 new language ca@valencian

!insertmacro MUI_LANGUAGE "Russian"

; Product name
LangString lng_Caption   ${LANG_RUSSIAN} "${PRODUCT_NAME} -- �������� ��������� ������� � �������� �������� �����"

; Button text "Next >" for the license page
LangString lng_LICENSE_BUTTON   ${LANG_RUSSIAN} "������ >"

; Bottom text for the license page
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_RUSSIAN} "$(^Name) ������� �� �������� GNU General Public License (GPL). �������� ������������ ��� ������������. $_CLICK"

;has been installed by different user
LangString lng_DIFFERENT_USER ${LANG_RUSSIAN} "Inkscape ���������� ������������� $0.$\r$\n���� �� ����������, ��������� ����� �� ����������� �������!$\r$\n������� � ������� ��� ������������ $0 � ���������� �����."

; want to uninstall before install
LangString lng_WANT_UNINSTALL_BEFORE ${LANG_RUSSIAN} "$R1 ��� �����������. $\n�� ������ ������� ���������� ������ ����� ���������� $(^Name) ?"

; press OK to continue press Cancel to abort
LangString lng_OK_CANCEL_DESC ${LANG_RUSSIAN} "$\n$\n������� OK ��� ����������� ��� CANCEL ��� ���������� ���������."

;you have no admin rigths
LangString lng_NO_ADMIN ${LANG_RUSSIAN} "� ��� ��� ���� ��������������.$\r$\n��������� Inkscape ��� ���� ������������� ����� �� ����������� �������.$\r$\n�� ����������� �������� ���� ���� �������������."

;win9x is not supported
LangString lng_NOT_SUPPORTED ${LANG_RUSSIAN} "Inkscape �� �������� � Windows 95/98/ME!$\r$\n����������� �������� �� ����� ���������."

; Full install type
LangString lng_Full $(LANG_RUSSIAN) "������"

; Optimal install type
LangString lng_Optimal $(LANG_RUSSIAN) "�����������"

; Minimal install type
LangString lng_Minimal $(LANG_RUSSIAN) "�����������"

; Core install section
LangString lng_Core $(LANG_RUSSIAN) "${PRODUCT_NAME}, �������� SVG (���������)"

; Core install section description
LangString lng_CoreDesc $(LANG_RUSSIAN) "�������� ����� � ���������� ${PRODUCT_NAME}"

; GTK+ install section
LangString lng_GTKFiles $(LANG_RUSSIAN) "����� ���������� GTK+ (���������)"

; GTK+ install section description
LangString lng_GTKFilesDesc $(LANG_RUSSIAN) "������������������ �������� ����������, ����������� ��� ${PRODUCT_NAME}"

; shortcuts install section
LangString lng_Shortcuts $(LANG_RUSSIAN) "������"

; shortcuts install section description
LangString lng_ShortcutsDesc $(LANG_RUSSIAN) "������ ��� ������� ${PRODUCT_NAME}"

; All user install section
LangString lng_Alluser $(LANG_RUSSIAN) "��� ���� �������������"

; All user install section description
LangString lng_AlluserDesc $(LANG_RUSSIAN) "���������� ��������� ��� ���� ������������� ����� ����������"

; Desktop section
LangString lng_Desktop $(LANG_RUSSIAN) "������� ����"

; Desktop section description
LangString lng_DesktopDesc $(LANG_RUSSIAN) "������� ����� ��� ${PRODUCT_NAME} �� ������� �����"

; Start Menu  section
LangString lng_Startmenu $(LANG_RUSSIAN) "���� �����"

; Start Menu section description
LangString lng_StartmenuDesc $(LANG_RUSSIAN) "������� ������ ${PRODUCT_NAME} � ���� �����"

; Quick launch section
LangString lng_Quicklaunch $(LANG_RUSSIAN) "������ �������� �������"

; Quick launch section description
LangString lng_QuicklaunchDesc $(LANG_RUSSIAN) "������� ����� ��� ${PRODUCT_NAME} � ������ �������� �������"

; File type association for editing
LangString lng_SVGWriter ${LANG_RUSSIAN} "��������� ����� SVG � ${PRODUCT_NAME}"

; File type association for editing description
LangString lng_SVGWriterDesc ${LANG_RUSSIAN} "������� ${PRODUCT_NAME} ���������� ������ SVG �� ���������"

; Context Menu
LangString lng_ContextMenu ${LANG_RUSSIAN} "����������� ����"

; Context Menu description
LangString lng_ContextMenuDesc ${LANG_RUSSIAN} "�������� ${PRODUCT_NAME} � ����������� ���� ��� ������ SVG"

; remove personal preferences
LangString lng_DeletePrefs ${LANG_RUSSIAN} "������� ������ ���������"

; remove personal preferences description
LangString lng_DeletePrefsDesc ${LANG_RUSSIAN} "������� ������ ���������, ���������� �� ���������� ������ ���������"


; Additional files section
LangString lng_Addfiles $(LANG_RUSSIAN) "�������������� �����"

; Additional files section description
LangString lng_AddfilesDesc $(LANG_RUSSIAN) "�������������� �����"

; Examples section
LangString lng_Examples $(LANG_RUSSIAN) "�������"

; Examples section description
LangString lng_ExamplesDesc $(LANG_RUSSIAN) "������� ������, ��������� � ${PRODUCT_NAME}"

; Tutorials section
LangString lng_Tutorials $(LANG_RUSSIAN) "�����"

; Tutorials section description
LangString lng_TutorialsDesc $(LANG_RUSSIAN) "����� �� ������������� ${PRODUCT_NAME}"


; Languages section
LangString lng_Languages $(LANG_RUSSIAN) "��������"

; Languages section dscription
LangString lng_LanguagesDesc $(LANG_RUSSIAN) "��������� ��������� ${PRODUCT_NAME} �� ������ �����"

LangString lng_am $(LANG_RUSSIAN) "am  ��������� (��������)"
LangString lng_az $(LANG_RUSSIAN) "az  ���������������"
LangString lng_be $(LANG_RUSSIAN) "be  �����������"
LangString lng_bg $(LANG_RUSSIAN) "bg  ����������"
LangString lng_bn $(LANG_RUSSIAN) "bn  Bengali"
LangString lng_ca $(LANG_RUSSIAN) "ca  �����������"
LangString lng_ca@valencia $(LANG_RUSSIAN) "ca@valencia  Valencian Catalan"
LangString lng_cs $(LANG_RUSSIAN) "cs  �������"
LangString lng_da $(LANG_RUSSIAN) "da  �������"
LangString lng_de $(LANG_RUSSIAN) "de  ��������"
LangString lng_dz $(LANG_RUSSIAN) "dz  �����-��"
LangString lng_el $(LANG_RUSSIAN) "el  ���������"
LangString lng_en $(LANG_RUSSIAN) "en  ����������"
LangString lng_en_AU $(LANG_RUSSIAN) "en_AU Australian English"
LangString lng_en_CA $(LANG_RUSSIAN) "en_CA ���������� (������)"
LangString lng_en_GB $(LANG_RUSSIAN) "en_GB ���������� (��������������)"
LangString lng_en_US@piglatin $(LANG_RUSSIAN) "en_US@piglatin ��������� ������"
LangString lng_eo $(LANG_RUSSIAN) "eo  Esperanto"
LangString lng_es $(LANG_RUSSIAN) "es  ���������"
LangString lng_es_MX $(LANG_RUSSIAN) "es_MX  ��������� (�������)"
LangString lng_et $(LANG_RUSSIAN) "et  ���������"
LangString lng_eu $(LANG_RUSSIAN) "eu  ��������"
LangString lng_fi $(LANG_RUSSIAN) "fi  �������"
LangString lng_fr $(LANG_RUSSIAN) "fr  �����������"
LangString lng_ga $(LANG_RUSSIAN) "ga  ����������"
LangString lng_gl $(LANG_RUSSIAN) "gl  �����������"
LangString lng_he $(LANG_RUSSIAN) "he  Hebrew"
LangString lng_hr $(LANG_RUSSIAN) "hr  ����������"
LangString lng_hu $(LANG_RUSSIAN) "hu  ����������"
LangString lng_id $(LANG_RUSSIAN) "id  Indonesian"
LangString lng_it $(LANG_RUSSIAN) "it  �����������"
LangString lng_ja $(LANG_RUSSIAN) "ja  ��������"
LangString lng_km $(LANG_RUSSIAN) "km  ���������"
LangString lng_ko $(LANG_RUSSIAN) "ko  ���������"
LangString lng_lt $(LANG_RUSSIAN) "lt  ���������"
LangString lng_mk $(LANG_RUSSIAN) "mk  �����������"
LangString lng_mn $(LANG_RUSSIAN) "mn  �����������"
LangString lng_ne $(LANG_RUSSIAN) "ne  ����������"
LangString lng_nb $(LANG_RUSSIAN) "nb  ���������� (������)"
LangString lng_nl $(LANG_RUSSIAN) "nl  �������"
LangString lng_nn $(LANG_RUSSIAN) "nn  ���������� (�������)"
LangString lng_pa $(LANG_RUSSIAN) "pa  �����������"
LangString lng_pl $(LANG_RUSSIAN) "po  ��������"
LangString lng_pt $(LANG_RUSSIAN) "pt  �������������"
LangString lng_pt_BR $(LANG_RUSSIAN) "pt_BR ������������� (��������)"
LangString lng_ro $(LANG_RUSSIAN) "ro  Romanian"
LangString lng_ru $(LANG_RUSSIAN) "ru  �������"
LangString lng_rw $(LANG_RUSSIAN) "rw  �����������"
LangString lng_sk $(LANG_RUSSIAN) "sk  ���������"
LangString lng_sl $(LANG_RUSSIAN) "sl  ����������"
LangString lng_sq $(LANG_RUSSIAN) "sq  ���������"
LangString lng_sr $(LANG_RUSSIAN) "sr  ��������"
LangString lng_sr@Latn $(LANG_RUSSIAN) "sr@Latn  �������� (��������)"
LangString lng_sv $(LANG_RUSSIAN) "sv  ��������"
LangString lng_th $(LANG_RUSSIAN) "th  �������"
LangString lng_tr $(LANG_RUSSIAN) "tr  ��������"
LangString lng_uk $(LANG_RUSSIAN) "uk  ����������"
LangString lng_vi $(LANG_RUSSIAN) "vi  �����������"
LangString lng_zh_CN $(LANG_RUSSIAN) "zh_CH  ��������� ����������"
LangString lng_zh_TW $(LANG_RUSSIAN) "zh_TW  ��������� ������������"




; uninstallation options
LangString lng_UInstOpt   ${LANG_RUSSIAN} "��������� �������� ��������� �� �������"

; uninstallation options subtitle
LangString lng_UInstOpt1  ${LANG_RUSSIAN} "��������� � ���, ��� ������� �������������� ���������"

; Ask to purge the personal preferences
LangString lng_PurgePrefs ${LANG_RUSSIAN} "��������� ������ ���������"

 	  	 
