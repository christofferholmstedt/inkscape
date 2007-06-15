; #######################################
; japanese.nsh
; japanese language strings for inkscape installer
; windows code page: 932
; Authors:
; Kenji Inoue kenz@oct.zaq.ne.jp
;
; february 15 2007 new language bn, en_AU, eo, id, ro
; april 11 2007 new language he

!insertmacro MUI_LANGUAGE "Japanese"

; Product name
LangString lng_Caption   ${LANG_JAPANESE} "${PRODUCT_NAME} -- Open Source Scalable Vector Graphics Editor"

; Button text "Next >" for the license page
LangString lng_LICENSE_BUTTON   ${LANG_JAPANESE} "���� >"

; Bottom text for the license page
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_JAPANESE} "$(^Name) �� GNU General Public License (GPL) �̉��Ń����[�X����܂��B�����܂ŎQ�l�̂��߂Ƀ��C�Z���X�������ɒ񎦂��܂��B$_CLICK"

;has been installed by different user
LangString lng_DIFFERENT_USER ${LANG_JAPANESE} "Inkscape �̓��[�U $0 �ɂ���ăC���X�g�[������Ă��܂��B$\r$\n���̂܂ܑ�����Ɛ���Ɋ������Ȃ���������܂���B$\r$\n$0 �Ń��O�C�����Ă���ēx���݂Ă��������B"

; want to uninstall before install
LangString lng_WANT_UNINSTALL_BEFORE ${LANG_JAPANESE} "$R1 �͊��ɃC���X�g�[������Ă��܂��B$\n$(^Name) ���C���X�g�[������O�ɈȑO�̃o�[�W�������폜���܂����H"

; press OK to continue press Cancel to abort
LangString lng_OK_CANCEL_DESC ${LANG_JAPANESE} "$\n$\nOK�������Čp�����邩CANCEL�������Ē��~���Ă��������B"

;you have no admin rigths
LangString lng_NO_ADMIN ${LANG_JAPANESE} "�Ǘ��Ҍ���������܂���B$\r$\n���ׂẴ��[�U�ɑ΂��� Inkscape �̃C���X�g�[���͐���Ɋ������Ȃ���������܂���B$\r$\n'���ׂẴ��[�U' �I�v�V�����̃`�F�b�N���O���Ă��������B"

;win9x is not supported
LangString lng_NOT_SUPPORTED ${LANG_JAPANESE} "Inkscape �� Windows 95/98/ME ��ł͓��삵�܂���I$\r$\n�ڂ����̓I�t�B�V���� Web �T�C�g���������������B"

; Full install type
LangString lng_Full $(LANG_JAPANESE) "���S"

; Optimal install type
LangString lng_Optimal $(LANG_JAPANESE) "�œK"

; Minimal install type
LangString lng_Minimal $(LANG_JAPANESE) "�ŏ�"

; Core install section
LangString lng_Core $(LANG_JAPANESE) "${PRODUCT_NAME} SVG Editor (�K�{)"

; Core install section description
LangString lng_CoreDesc $(LANG_JAPANESE) "${PRODUCT_NAME} �̃R�A�t�@�C����DLL"

; GTK+ install section
LangString lng_GTKFiles $(LANG_JAPANESE) "GTK+ �����^�C�����i�K�{�j"

; GTK+ install section description
LangString lng_GTKFilesDesc $(LANG_JAPANESE) "�}���`�v���b�g�t�H�[���Ή� GUI �c�[���L�b�g�i${PRODUCT_NAME} ���g�p�j"

; shortcuts install section
LangString lng_Shortcuts $(LANG_JAPANESE) "�V���[�g�J�b�g"

; shortcuts install section description
LangString lng_ShortcutsDesc $(LANG_JAPANESE) "${PRODUCT_NAME} ���J�n���邽�߂̃V���[�g�J�b�g"

; All user install section
LangString lng_Alluser $(LANG_JAPANESE) "���ׂẴ��[�U"

; All user install section description
LangString lng_AlluserDesc $(LANG_JAPANESE) "���̃R���s���[�^���g�����ׂĂ̐l�ɂ��̃A�v���P�[�V�������C���X�g�[���i���ׂẴ��[�U�j"

; Desktop section
LangString lng_Desktop $(LANG_JAPANESE) "�f�X�N�g�b�v"

; Desktop section description
LangString lng_DesktopDesc $(LANG_JAPANESE) "${PRODUCT_NAME} �ւ̃V���[�g�J�b�g���f�X�N�g�b�v�ɍ쐬"

; Start Menu  section
LangString lng_Startmenu $(LANG_JAPANESE) "�X�^�[�g���j���["

; Start Menu section description
LangString lng_StartmenuDesc $(LANG_JAPANESE) "�X�^�[�g���j���[�� ${PRODUCT_NAME} �̍��ڂ��쐬"

; Quick launch section
LangString lng_Quicklaunch $(LANG_JAPANESE) "�N�C�b�N�N��"

; Quick launch section description
LangString lng_QuicklaunchDesc $(LANG_JAPANESE) "${PRODUCT_NAME} �ւ̃V���[�g�J�b�g���N�C�b�N�N���c�[���o�[�ɍ쐬"

; File type association for editing
LangString lng_SVGWriter ${LANG_JAPANESE} "SVG �t�@�C���� ${PRODUCT_NAME} �ŊJ��"

; File type association for editing description
LangString lng_SVGWriterDesc ${LANG_JAPANESE} "SVG �t�@�C���̕W���G�f�B�^�� ${PRODUCT_NAME} ��ݒ�"

; Context Menu
LangString lng_ContextMenu ${LANG_JAPANESE} "�R���e�L�X�g���j���["

; Context Menu description
LangString lng_ContextMenuDesc ${LANG_JAPANESE} "SVG �t�@�C���̃R���e�L�X�g���j���[�� ${PRODUCT_NAME} ��ǉ�"

; remove personal preferences
LangString lng_DeletePrefs ${LANG_JAPANESE} "�l�ݒ���폜"

; remove personal preferences description
LangString lng_DeletePrefsDesc ${LANG_JAPANESE} "�ȑO�̃C���X�g�[������������p�����l�ݒ���폜"


; Additional files section
LangString lng_Addfiles $(LANG_JAPANESE) "�ǉ��t�@�C��"

; Additional files section description
LangString lng_AddfilesDesc $(LANG_JAPANESE) "�ǉ��t�@�C��"

; Examples section
LangString lng_Examples $(LANG_JAPANESE) "�T���v���t�@�C��"

; Examples section description
LangString lng_ExamplesDesc $(LANG_JAPANESE) "${PRODUCT_NAME} �̃T���v���t�@�C��"

; Tutorials section
LangString lng_Tutorials $(LANG_JAPANESE) "�`���[�g���A��"

; Tutorials section description
LangString lng_TutorialsDesc $(LANG_JAPANESE) "${PRODUCT_NAME} �̃`���[�g���A��"


; Languages section
LangString lng_Languages $(LANG_JAPANESE) "�|��"

; Languages section dscription
LangString lng_LanguagesDesc $(LANG_JAPANESE) "${PRODUCT_NAME} �̂��܂��܂̖|��t�@�C�����C���X�g�[��"

LangString lng_am $(LANG_JAPANESE) "am  �A���n����"
LangString lng_az $(LANG_JAPANESE) "az  �A�[���o�C�W������"
LangString lng_be $(LANG_JAPANESE) "be  �x�����[�V��"
LangString lng_bg $(LANG_JAPANESE) "bg  �u���K���A��"
LangString lng_bn $(LANG_JAPANESE) "bn  Bengali"
LangString lng_ca $(LANG_JAPANESE) "ca  �J�^���j�A��"
LangString lng_cs $(LANG_JAPANESE) "cs  �`�F�R��"
LangString lng_da $(LANG_JAPANESE) "da  �f���}�[�N��"
LangString lng_de $(LANG_JAPANESE) "de  �h�C�c��"
LangString lng_dz $(LANG_JAPANESE) "dz  �]���J��"
LangString lng_el $(LANG_JAPANESE) "el  �M���V����"
LangString lng_en $(LANG_JAPANESE) "en  �p��"
LangString lng_en_AU $(LANG_JAPANESE) "en_AU Australian English"
LangString lng_en_CA $(LANG_JAPANESE) "en_CA �p��i�J�i�_�j"
LangString lng_en_GB $(LANG_JAPANESE) "en_GB �p��i�p���j"
LangString lng_en_US@piglatin $(LANG_JAPANESE) "�p��i�s�b�O���e���A�č��j"
LangString lng_eo $(LANG_JAPANESE) "eo  Esperanto"
LangString lng_es $(LANG_JAPANESE) "es  �X�y�C����"
LangString lng_es_MX $(LANG_JAPANESE) "es_MX  �X�y�C����i���L�V�R�j"
LangString lng_et $(LANG_JAPANESE) "et  �G�X�g�j�A��"
LangString lng_eu $(LANG_JAPANESE) "eu  �o�X�N��"
LangString lng_fi $(LANG_JAPANESE) "fi  �t�B�������h��"
LangString lng_fr $(LANG_JAPANESE) "fr  �t�����X��"
LangString lng_ga $(LANG_JAPANESE) "ga  �A�C�������h��"
LangString lng_gl $(LANG_JAPANESE) "gl  �K���V�A��"
LangString lng_he $(LANG_JAPANESE) "he  Hebrew"
LangString lng_hr $(LANG_JAPANESE) "hr  �N���A�`�A��"
LangString lng_hu $(LANG_JAPANESE) "hu  �n���K���[��"
LangString lng_id $(LANG_JAPANESE) "id  Indonesian"
LangString lng_it $(LANG_JAPANESE) "it  �C�^���A��"
LangString lng_ja $(LANG_JAPANESE) "ja  ���{��"
LangString lng_km $(LANG_JAPANESE) "km  �N���[����"
LangString lng_ko $(LANG_JAPANESE) "ko  �؍���"
LangString lng_lt $(LANG_JAPANESE) "lt  ���g�A�j�A��"
LangString lng_mk $(LANG_JAPANESE) "mk  �}�P�h�j�A��"
LangString lng_mn $(LANG_JAPANESE) "mn  �����S����"
LangString lng_ne $(LANG_JAPANESE) "ne  �l�p�[����"
LangString lng_nb $(LANG_JAPANESE) "nb  �m���E�F�[��i�u�[�N���[���j"
LangString lng_nl $(LANG_JAPANESE) "nl  �I�����_��"
LangString lng_nn $(LANG_JAPANESE) "nn  �m���E�F�[��i�j�[�m�V�N�j"
LangString lng_pa $(LANG_JAPANESE) "pa  �p���W���u��"
LangString lng_pl $(LANG_JAPANESE) "po  �|�[�����h��"
LangString lng_pt $(LANG_JAPANESE) "pt  �|���g�K����"
LangString lng_pt_BR $(LANG_JAPANESE) "pt_BR �|���g�K����i�u���W���j"
LangString lng_ro $(LANG_JAPANESE) "ro  Romanian"
LangString lng_ru $(LANG_JAPANESE) "ru  ���V�A��"
LangString lng_rw $(LANG_JAPANESE) "rw  �L�����������_��"
LangString lng_sk $(LANG_JAPANESE) "sk  �X���o�L�A��"
LangString lng_sl $(LANG_JAPANESE) "sl  �X���x�j�A��"
LangString lng_sq $(LANG_JAPANESE) "sq  �A���o�j�A��"
LangString lng_sr $(LANG_JAPANESE) "sr  �Z���r�A��"
LangString lng_sr@Latn $(LANG_JAPANESE) "sr@Latn  �Z���r�A��i���e���j"
LangString lng_sv $(LANG_JAPANESE) "sv  �X�E�F�[�f����"
LangString lng_th $(LANG_JAPANESE) "th  �^�C��"
LangString lng_tr $(LANG_JAPANESE) "tr  �g���R��"
LangString lng_uk $(LANG_JAPANESE) "uk  �E�N���C�i��"
LangString lng_vi $(LANG_JAPANESE) "vi  �x�g�i����"
LangString lng_zh_CN $(LANG_JAPANESE) "zh_CH  ������i�ȑ̎��j"
LangString lng_zh_TW $(LANG_JAPANESE) "zh_TW  ������i�ɑ̎��j"




; uninstallation options
LangString lng_UInstOpt   ${LANG_JAPANESE} "�A���C���X�g�[���I�v�V����"

; uninstallation options subtitle
LangString lng_UInstOpt1  ${LANG_JAPANESE} "�K�v�ł���Έȉ��̃I�v�V������I�����Ă�������"

; Ask to purge the personal preferences
LangString lng_PurgePrefs ${LANG_JAPANESE} "�l�ݒ���c��"

 	  	 
