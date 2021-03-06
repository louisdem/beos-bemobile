
#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Font.h>
#include <StatusBar.h>
#include <StringView.h>
#include "ColumnListView.h"
#include "globals.h"
#include "phonelist.h"
#include "dialeditpbook.h"

#include <stdio.h>

const uint32	CRLIST_INV		= 'CR00';
const uint32	CRLIST_SEL 		= 'CR01';
const uint32	CRREFRESH		= 'CR02';
const uint32	CRDELETE		= 'CR03';
const uint32	CRDIAL			= 'CR04';
const uint32	CREDIT			= 'CR05';
const uint32	CRNEW			= 'CR06';

phoneListView::phoneListView(BRect r, const char *slot, const char *name, GSM *g) : mobileView(r, name, g) {

	memSlot = slot;
	editable = false;

	BFont font(be_plain_font);
	float maxw, totalw = 0;
	BRect r = this->MyBounds();
	r.InsetBy(10,15);
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= 100;

	// add column list
	int flags = CLV_TELL_ITEMS_WIDTH|CLV_HEADER_TRUNCATE|CLV_SORT_KEYABLE;
	CLVContainerView *containerView;
	list = new ColumnListView(r, &containerView, NULL, B_FOLLOW_TOP_BOTTOM|B_FOLLOW_LEFT_RIGHT,
		B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE, B_SINGLE_SELECTION_LIST, false, true, true, true,
		B_FANCY_BORDER);
	maxw = font.StringWidth("XXXX"); totalw += maxw;
	list->AddColumn(new CLVColumn("#", maxw, flags));
	maxw = font.StringWidth("+999999999") + 20; totalw += maxw;
	// these are required fields
	list->AddColumn(new CLVColumn(_("Number"), maxw, flags));
	maxw = font.StringWidth("MMMMMMMMMMMMMMMMMMMM") + 20; totalw += maxw;
	list->AddColumn(new CLVColumn(_("Name"), maxw, flags));
	// go through fields
	struct pbSlot *sl = gsm->getPBSlot(memSlot.String());
	struct pbField *pf;
	bool last;
	int j = sl->fields->CountItems();
	for (int i=2;i<j;i++) {	// skip number and name
		last = (i+1 == j);
		pf = (struct pbField*)sl->fields->ItemAt(i);
		switch (pf->type) {
			case GSM::PF_PHONEEMAIL:
			case GSM::PF_PHONE:
				maxw = font.StringWidth("9")*(pf->max)+20;
				break;
			case GSM::PF_TEXT:
				maxw = font.StringWidth("M")*(pf->max)+20;
				break;
			case GSM::PF_DATE:
				maxw = font.StringWidth("MM/MM/MMM");
				break;
			case GSM::PF_COMBO:
				maxw = font.StringWidth("MMMMMMM")+20;
				break;
			case GSM::PF_BOOL:
				maxw = font.StringWidth("MMM")+20;
				break;
			default:
				maxw = font.StringWidth("M")*3+20;
				break;
		}
		if ((last) && (maxw < r.right-B_V_SCROLL_BAR_WIDTH-totalw))
				maxw = r.right-B_V_SCROLL_BAR_WIDTH-totalw;
		totalw += maxw;
		list->AddColumn(new CLVColumn(pf->name.String(), maxw, flags));
	}
	list->SetSortFunction(CLVEasyItem::CompareItems);
	this->AddChild(containerView);
	list->SetInvocationMessage(new BMessage(CRLIST_INV));
	list->SetSelectionMessage(new BMessage(CRLIST_SEL));

	r = this->MyBounds();
	r.InsetBy(10,15);
	r.top = r.bottom - font.Size()*4 - 30;
	r.bottom = r.top + font.Size()*2;
	this->AddChild(progress = new BStatusBar(r, "crStatusBar"));
	progress->SetResizingMode(B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM);
	progress->SetMaxValue(100);
	progress->Hide();

	r = this->MyBounds();
	r.InsetBy(20,20);
	BRect s;
	s = r; s.top = s.bottom - font.Size()*2;
	float len = s.Width()/5;	
	s.right = s.left + len - 10;
	this->AddChild(but_refresh = new BButton(s, "crRefresh", _("Refresh"), new BMessage(CRREFRESH), B_FOLLOW_LEFT|B_FOLLOW_BOTTOM));
	s.OffsetBy(len,0);
	this->AddChild(but_new = new BButton(s, "crNew", _("New"), new BMessage(CRNEW), B_FOLLOW_LEFT|B_FOLLOW_BOTTOM));
	but_new->Hide();
	s.OffsetBy(len,0);
	this->AddChild(but_edit = new BButton(s, "crEdit", _("Edit"), new BMessage(CREDIT), B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM));
	but_edit->Hide();
	s.OffsetBy(len,0);
	this->AddChild(but_del = new BButton(s, "crDelete", _("Delete"), new BMessage(CRDELETE), B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM));
	but_del->Hide();
	s.OffsetBy(len,0);
	this->AddChild(but_dial = new BButton(s, "crDial", _("Dial"), new BMessage(CRDIAL), B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM));
}

void phoneListView::clearList(void) {
	if (list->CountItems()>0) {
		CLVEasyItem *anItem;
		for (int i=0; (anItem=(phoneSlotListItem*)list->ItemAt(i)); i++)
			delete anItem;
		if (!list->IsEmpty())
			list->MakeEmpty();
	}
}

void phoneListView::fillList(void) {
	struct pbSlot *sl = gsm->getPBSlot(memSlot.String());
	struct pbNum *num;

	clearList();

	int j = sl->pb->CountItems();
	for (int i=0;i<j;i++) {
		num = (struct pbNum*)sl->pb->ItemAt(i);
		list->AddItem(new phoneSlotListItem(num,gsm));
	}
}

void phoneListView::fullListRefresh(void) {
	progress->Reset();
	progress->Show();

	progress->Update(0, _("Reading phonebook..."));

	gsm->getPBList(memSlot.String());

	progress->Hide();
}

void phoneListView::Show(void) {
	BView::Show();
	BView::Flush();
	BView::Sync();
	if (list->CountItems()==0) {
		if (gsm->getPBSlot(memSlot.String())->pb->CountItems() == 0)
			fullListRefresh();
	}
	fillList();
}

void phoneListView::MessageReceived(BMessage *Message) {
	switch (Message->what) {
		case CRREFRESH:
			fullListRefresh();
			fillList();
			break;
		case CRDIAL:
			{	int i = list->CurrentSelection(0);

				if (i>=0) {
					struct pbNum *p = ((phoneSlotListItem*)list->ItemAt(i))->Num();
					if ((p->type != GSM::PB_PHONE) && (p->type != GSM::PB_INTLPHONE)) {
						BAlert *err = new BAlert(APP_NAME, _("Selected item is not a phone number"), _("Ok"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
						err->Go();
					} else {
						gsm->dial(p->number.String());
					}
				}
				break;
			}
		case CRDELETE:
			{	int i = list->CurrentSelection(0);

				if (i>=0) {
					BAlert *ask = new BAlert(APP_NAME, _("Do you really want to delete this entry?"), _("Yes"), _("No"), NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					if (ask->Go() == 0) {
						struct pbNum *p = ((phoneSlotListItem*)list->ItemAt(i))->Num();
						printf("removing:[%s]\n",p->number.String());
						if (gsm->removePBItem(p) == 0)
							list->RemoveItem(i);
					}
				}
				break;
			}
		case CRNEW:
			{
				dialEditPB *dn;
				dn = new dialEditPB(memSlot.String(), gsm);
				break;
			}
		case CRLIST_INV:
//			printf("inv\n");
		case CREDIT:
			{	int i = list->CurrentSelection(0);
				if ((i>=0) && editable) {
					dialEditPB *dn;
					dn = new dialEditPB(memSlot.String(),gsm,((phoneSlotListItem*)list->ItemAt(i))->Num());
				}
				break;
			}
		case CRLIST_SEL:
//			printf("sel\n");
//			break;
		default:
			mobileView::MessageReceived(Message);
			break;
	}
}


phoneSlotListItem::phoneSlotListItem(struct pbNum *num, GSM *gsm) : CLVEasyItem(0, false, false, 20.0) {
	BString tmp;

	fNum = num;
	fId = num->id;
	tmp = ""; tmp << num->id;
	SetColumnContent(0,tmp.String());

	struct pbSlot *sl = gsm->getPBSlot(num->slot.String());
	struct pbField *pf;
	union pbVal *v;
	int j = sl->fields->CountItems();
	for (int i=0;i<j;i++) {
		pf = (struct pbField*)sl->fields->ItemAt(i);
		v = (union pbVal*)num->attr->ItemAt(i);
		switch (pf->type) {
			case GSM::PF_PHONEEMAIL:
			case GSM::PF_PHONE:
			case GSM::PF_TEXT:
			case GSM::PF_DATE:
				SetColumnContent(i+1,v->text->String());
				break;
			case GSM::PF_BOOL:
				tmp = (v->b ? _("Yes") : _("No"));
				SetColumnContent(i+1,tmp.String());
				break;
			case GSM::PF_COMBO:
				int l = pf->cb->CountItems();
				struct pbCombo *pc;
				for (int k=0; k<l; k++) {
					pc = (struct pbCombo*)pf->cb->ItemAt(k);
					if (pc->v == v->v) {
						SetColumnContent(i+1,pc->text.String());
						break;
					}
				}
				break;
		}
	}
}
