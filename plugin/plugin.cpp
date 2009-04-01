/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "plugin.h"
//#define MIME_TYPES_HANDLED  "application/x-shockwave-flash"
#define MIME_TYPES_HANDLED  "application/x-lightspark"
#define PLUGIN_NAME    "Shockwave Flash"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED":swf:"PLUGIN_NAME
#define PLUGIN_DESCRIPTION "Shockwave Flash 9.0 r31"

using namespace std;

char* NPP_GetMIMEDescription(void)
{
  return(MIME_TYPES_DESCRIPTION);
}

/////////////////////////////////////
// general initialization and shutdown
//
NPError NS_PluginInitialize()
{
  return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
}

// get values per plugin
NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue)
{
  NPError err = NPERR_NO_ERROR;
  switch (aVariable) {
    case NPPVpluginNameString:
      *((char **)aValue) = PLUGIN_NAME;
      break;
    case NPPVpluginDescriptionString:
      *((char **)aValue) = PLUGIN_DESCRIPTION;
      break;
    default:
      err = NPERR_INVALID_PARAM;
      break;
  }
  return err;
}

/////////////////////////////////////////////////////////////
//
// construction and destruction of our plugin instance object
//
nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct)
{
  if(!aCreateDataStruct)
    return NULL;

  nsPluginInstance * plugin = new nsPluginInstance(aCreateDataStruct->instance);
  return plugin;
}

void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin)
{
  if(aPlugin)
    delete (nsPluginInstance *)aPlugin;
}

////////////////////////////////////////
//
// nsPluginInstance class implementation
//
nsPluginInstance::nsPluginInstance(NPP aInstance) : nsPluginInstanceBase(),
  mInstance(aInstance),
  mInitialized(FALSE),
  mWindow(0),
  mXtwidget(0),
  mFontInfo(0),
  swf_stream(&swf_buf),
  pt(swf_stream),
  rt(NULL)
{
}

nsPluginInstance::~nsPluginInstance()
{
}

static void
xt_event_handler(Widget xtwidget, nsPluginInstance *plugin, XEvent *xevent, Boolean *b)
{
  switch (xevent->type) {
    case Expose:
      // get rid of all other exposure events
      if (plugin) {
        //while(XCheckTypedWindowEvent(plugin->Display(), plugin->Window(), Expose, xevent));
        plugin->draw();
      }
      default:
        break;
  }
}

extern SystemState sys;

void nsPluginInstance::draw()
{
	if(rt==NULL)
		return;
	if(sys.clip.frames.size()>10)
		rt->draw(&sys.clip.frames[9]);
}

NPBool nsPluginInstance::init(NPWindow* aWindow)
{
  if(aWindow == NULL)
    return FALSE;
  
  if (SetWindow(aWindow))
  mInitialized = TRUE;
	
  return mInitialized;
}

void nsPluginInstance::shut()
{
  mInitialized = FALSE;
}

const char * nsPluginInstance::getVersion()
{
  return NPN_UserAgent(mInstance);
}

NPError nsPluginInstance::GetValue(NPPVariable aVariable, void *aValue)
{
  NPError err = NPERR_NO_ERROR;
  switch (aVariable) {
    case NPPVpluginNameString:
    case NPPVpluginDescriptionString:
      return NS_PluginGetValue(aVariable, aValue) ;
      break;
    default:
      err = NPERR_INVALID_PARAM;
      break;
  }
  return err;

}

NPError nsPluginInstance::SetWindow(NPWindow* aWindow)
{
  if(aWindow == NULL)
    return FALSE;

  mX = aWindow->x;
  mY = aWindow->y;
  mWidth = aWindow->width;
  mHeight = aWindow->height;
  if (mWindow == (Window) aWindow->window) {
    // The page with the plugin is being resized.
    // Save any UI information because the next time
    // around expect a SetWindow with a new window id.
  }
  else
  {
    mWindow = (Window) aWindow->window;
    NPSetWindowCallbackStruct *ws_info = (NPSetWindowCallbackStruct *)aWindow->ws_info;
    mDisplay = ws_info->display;
    mVisual = ws_info->visual;
    mDepth = ws_info->depth;
    mColormap = ws_info->colormap;

    printf("Window visual 0x%x\n",XVisualIDFromVisual(mVisual));

    if (!mFontInfo) {
      if (!(mFontInfo = XLoadQueryFont(mDisplay, "9x15")))
        printf("Cannot open 9X15 font\n");
    }

    NPAPI_params* p=new NPAPI_params;

    p->visual=XVisualIDFromVisual(mVisual);
    p->window=mWindow;

    if(rt!=NULL)
    {
	    cout << "destroy old context" << endl;
	    abort();
    }
    rt=new RenderThread(NPAPI,p);

    // add xt event handler
    Widget xtwidget = XtWindowToWidget(mDisplay, mWindow);
    if (xtwidget && mXtwidget != xtwidget) {
      mXtwidget = xtwidget;
      long event_mask = ExposureMask;
      XSelectInput(mDisplay, mWindow, event_mask);
      XtAddEventHandler(xtwidget, event_mask, False, (XtEventHandler)xt_event_handler, this);
    }
  }
  draw();
  return TRUE;
}

NPError nsPluginInstance::NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype)
{
	*stype=NP_NORMAL;
	return NPERR_NO_ERROR; 
}

SystemState sys;

int32 nsPluginInstance::Write(NPStream *stream, int32 offset, int32 len, void *buffer)
{
	return swf_buf.sputn((char*)buffer,len);
}
