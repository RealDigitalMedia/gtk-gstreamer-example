#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gst/gst.h>

#include <gst/video/videooverlay.h>

#define PLAY_FACTORY             "play"
#define PLAY_FACTORY_NAME        "playbin"

#define VIDEO_SINK_FORCE_ASPECT_RATIO  "force-aspect-ratio"
#define PLAYBIN_URI                    "uri"

GtkWindow* myGtkWindow;
GstElement* myPlayBin;
char* myVideo;
GMainLoop* myLoop;

void loadVideo();
void moveToState(GstState requestedState_i);

// ==========================================================================
// According to:
//      http://www.gstreamer.net/data/doc/gstreamer/head/gst-plugins-base-libs/html/gst-plugins-base-libs-gstxoverlay.html
//
// this sync callback is the correct way to associate an xwindow id to
// the pipeline.  This handler is called when new messages arrive
// on the bus and is called in the same thread context as the
// posting object, so be careful of threading issues.
GstBusSyncReply ourSyncBusHandler(GstBus*     bus_i,
                                  GstMessage* message_i,
                                  gpointer    userData_i)
{
  // Set the window handle if required
  if (gst_is_video_overlay_prepare_window_handle_message(message_i))
  {
    GstVideoOverlay* overlay = GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message_i));
    
    GdkWindow* gdkWindow = gtk_widget_get_window(GTK_WIDGET(myGtkWindow));
    
    gulong windowID = gdkWindow ? gdk_x11_window_get_xid(gdkWindow) : 0;
    
    if (windowID != 0)
    {
      gst_video_overlay_set_window_handle(overlay, windowID);
    }
  }
  
  // Tell GStreamer we did not handle and unref the message
  return GST_BUS_PASS;
}

// ==========================================================================
// This handler responds to all of the message sent from GStreamer but are
// all delegated to the main glib loop, thus there should not be any threading
// issues within this code. NOTE however that the sync handler might be called
// during this code as well, so beware of interactions with that code.
guint ourASyncBusHandler(GstBus*     bus_i,
                         GstMessage* message_i,
                         gpointer    userData_i)
{
  GstMessageType messageType = GST_MESSAGE_TYPE(message_i);
  
  if (messageType == GST_MESSAGE_EOS) {
    loadVideo();
  }
  
  // Return TRUE to continue to process messages
  return TRUE;
}

// ==========================================================================
void moveToState(GstState requestedState_i)
{
  gst_element_set_state(GST_ELEMENT(myPlayBin), requestedState_i);
}

// ==========================================================================
void loadVideo()
{
  GstBus* bus;

  if (myPlayBin) {
    moveToState(GST_STATE_NULL);

    // Unreference the playbin
    g_object_unref(myPlayBin);

    // Clear the bus handler so we do not receive more callbacks
    bus = gst_pipeline_get_bus(GST_PIPELINE(myPlayBin));
    gst_bus_remove_watch(bus);
    gst_bus_set_sync_handler(bus, (GstBusSyncHandler)NULL, NULL, NULL);
    gst_object_unref(bus);
  }

  myPlayBin = gst_element_factory_make(PLAY_FACTORY_NAME, PLAY_FACTORY);
  
  // Sets the playbin to automatically stretch the video to fit the window
  g_object_set(G_OBJECT(myPlayBin),
                  VIDEO_SINK_FORCE_ASPECT_RATIO, FALSE,
                  NULL);
  
  g_object_set(G_OBJECT(myPlayBin), PLAYBIN_URI, myVideo, NULL);

  // Connect to the bus
  bus = gst_pipeline_get_bus(GST_PIPELINE(myPlayBin));
  
  // Add an asynchronous watch for all GStreamer messages. We want
  // to keep this to a minimum since these callbacks can happen
  // from a separate thread than the main glib thread where *ALL*
  // of our code is dispatched from. Thus this is a spot where
  // race conditions, reference counting, and memory errors can
  // be introduced.
  gst_bus_add_watch(bus, (GstBusFunc)ourASyncBusHandler, myLoop);
  
  // Set a synchronous handler just for the GStreamer events that must
  // be handled immediately.
  gst_bus_set_sync_handler(bus, (GstBusSyncHandler)ourSyncBusHandler, NULL, NULL);
  
  // Don't hold a reference to the bus
  gst_object_unref(bus);

  moveToState(GST_STATE_PLAYING);
}


// ==========================================================================
int main(int argc, char** argv)
{
  gtk_init(&argc, &argv);
  gst_init(0, NULL);
  
  myVideo = argv[1];
  
  /* create an event loop */
  myLoop = g_main_loop_new (NULL, FALSE);
  
  myGtkWindow = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  
  gtk_window_set_title(myGtkWindow, "Demo App");
  gtk_window_set_decorated(myGtkWindow, false);
  gtk_window_set_gravity(myGtkWindow, GDK_GRAVITY_STATIC);
  gtk_widget_set_redraw_on_allocate(GTK_WIDGET(myGtkWindow), false);
  
  loadVideo();
  
  Display* display = XOpenDisplay((char *)0);
  Screen*  screen  = DefaultScreenOfDisplay(display);
  
  int width  = XWidthOfScreen(screen);
  int height = XHeightOfScreen(screen);
  XCloseDisplay(display);
  
  gtk_window_resize(myGtkWindow, width, height);
  gtk_widget_show(GTK_WIDGET(myGtkWindow));
  
  g_main_loop_run (myLoop);
  
  return 0;
}

