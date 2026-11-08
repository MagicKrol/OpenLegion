package org.legion.game;

import android.os.Bundle;
import android.os.Handler;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.view.Gravity;
import android.graphics.Color;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.Button;
import android.widget.TextView;
import android.util.TypedValue;
import android.app.AlertDialog;
import android.text.InputFilter;
import android.widget.EditText;
import android.util.Log;
import org.libsdl.app.SDLActivity;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

public final class LegionActivity extends SDLActivity {
    private static native void nativeSetWorkingDirectory(String path);
    private static native void nativeEnableTestWorld();
    private static native void nativeEnableTestCity(boolean dialogue);
    private static native void nativeEnableTestStats();
    private static native void nativeEnableTestShop(boolean granary);
    private static native void nativeSetZoom(float scale, float x, float y);
    private static native void nativePointer(int phase, float x, float y, int button);
    private static native void nativeTacticalTouchBegin(float x, float y);
    private static native boolean nativeWorldMapActive();
    private static native boolean nativeTacticalSceneActive();
    private static native void nativeRequestRefresh();
    private static native void nativeWorldMapDrag(float dx, float dy);
    private static native void nativeSetInputResult(String text, boolean accepted);
    private static native void nativeSetMapChoice(int choice);
    private static native boolean nativeTargetSelectionActive();
    private static native void nativeSetMoveDecision(int decision);
    private static LegionActivity active;
    private final Handler touchHandler = new Handler();
    private ScaleGestureDetector scaleDetector;
    private LinearLayout moveControls;
    private boolean routingMoveControls;
    private float zoom = 1f, panX = 0f, panY = 0f;
    private float lastX, lastY, downX, downY, previousFocusX, previousFocusY;
    private boolean multiTouch, moved, rightPressed;
    private final Runnable longPress = () -> {
        if (!multiTouch && !moved) {
            rightPressed = true;
            nativePointer(0, downX / touchWidth(), downY / touchHeight(), 2);
        }
    };
    private View touchSurface() { return mSurface != null ? (View)mSurface : getWindow().getDecorView(); }
    private float touchWidth() { return Math.max(1, touchSurface().getWidth()); }
    private float touchHeight() { return Math.max(1, touchSurface().getHeight()); }
    private float surfaceX(float windowX) {
        int[] location=new int[2]; touchSurface().getLocationInWindow(location);
        return windowX-location[0];
    }
    private float surfaceY(float windowY) {
        int[] location=new int[2]; touchSurface().getLocationInWindow(location);
        return windowY-location[1];
    }
    private void applyZoom() {
        float screenAspect=touchWidth()/touchHeight();
        float gameAspect=640f/520f;
        float baseWidth=Math.min(1f,gameAspect/screenAspect);
        float baseHeight=Math.min(1f,screenAspect/gameAspect);
        float limitX=Math.max(0f,(baseWidth*zoom-1f)/2f);
        float limitY=Math.max(0f,(baseHeight*zoom-1f)/2f);
        panX=Math.max(-limitX,Math.min(limitX,panX));
        panY=Math.max(-limitY,Math.min(limitY,panY));
        nativeSetZoom(zoom, panX, panY);
    }
    public static void resetZoomForScene() {
        LegionActivity activity=active;
        if (activity==null) return;
        activity.runOnUiThread(() -> {
            activity.zoom=1f; activity.panX=0f; activity.panY=0f;
            activity.applyZoom();
        });
    }
    public static void showMoveControls(boolean show, boolean polish) {
        LegionActivity activity=active;
        if (activity==null) { if (show) nativeSetMoveDecision(-1); return; }
        activity.runOnUiThread(() -> {
            if (!show) {
                if (activity.moveControls!=null) {
                    ((android.view.ViewGroup)activity.moveControls.getParent()).removeView(activity.moveControls);
                    activity.moveControls=null;
                }
                return;
            }
            if (activity.moveControls!=null) return;
            LinearLayout panel=new LinearLayout(activity);
            panel.setOrientation(LinearLayout.HORIZONTAL);
            panel.setGravity(Gravity.CENTER);
            panel.setPadding(12,4,12,4);
            panel.setBackgroundColor(Color.rgb(46,35,25));
            Button cancel=new Button(activity);
            cancel.setText(polish ? "Anuluj ruch" : "Cancel move");
            cancel.setTextColor(Color.rgb(238,205,122));
            cancel.setOnClickListener(v -> nativeSetMoveDecision(-1));
            Button confirm=new Button(activity);
            confirm.setText(polish ? "Zatwierdź ruch" : "Confirm move");
            confirm.setTextColor(Color.rgb(238,205,122));
            confirm.setOnClickListener(v -> nativeSetMoveDecision(1));
            panel.addView(cancel,new LinearLayout.LayoutParams(0,LinearLayout.LayoutParams.WRAP_CONTENT,1));
            panel.addView(confirm,new LinearLayout.LayoutParams(0,LinearLayout.LayoutParams.WRAP_CONTENT,1));
            activity.moveControls=panel;
            FrameLayout.LayoutParams layout=new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,FrameLayout.LayoutParams.WRAP_CONTENT,Gravity.BOTTOM);
            activity.addContentView(panel,layout);
        });
    }
    public static void requestMapChoice(String[] labels) {
        requestChoice("Wybierz obiekt",labels,"Anuluj",false);
    }
    public static void requestChoice(String title, String[] labels, String cancel, boolean compact) {
        LegionActivity activity=active;
        if(activity==null) { nativeSetMapChoice(-1); return; }
        activity.runOnUiThread(() -> {
            AlertDialog.Builder builder=new AlertDialog.Builder(activity).setTitle(title)
                .setNegativeButton(cancel,(dialog,which) -> nativeSetMapChoice(-1))
                .setOnCancelListener(dialog -> nativeSetMapChoice(-1));
            if (compact) {
                float density=activity.getResources().getDisplayMetrics().density;
                LinearLayout rows=new LinearLayout(activity);
                rows.setOrientation(LinearLayout.VERTICAL);
                rows.setPadding(Math.round(16*density),0,Math.round(16*density),0);
                TypedValue selectable=new TypedValue();
                activity.getTheme().resolveAttribute(android.R.attr.selectableItemBackground,selectable,true);
                AlertDialog dialog=builder.setView(rows).create();
                for (int i=0;i<labels.length;++i) {
                    final int selected=i;
                    TextView row=new TextView(activity);
                    row.setText(labels[i]);
                    row.setTextColor(Color.WHITE);
                    row.setTextSize(15);
                    row.setSingleLine(true);
                    row.setGravity(Gravity.CENTER_VERTICAL);
                    row.setPadding(Math.round(8*density),0,Math.round(8*density),0);
                    row.setBackgroundResource(selectable.resourceId);
                    row.setOnClickListener(v -> { dialog.dismiss(); nativeSetMapChoice(selected); });
                    rows.addView(row,new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT,Math.round(40*density)));
                }
                dialog.show();
            } else builder.setItems(labels,(dialog,which) -> nativeSetMapChoice(which)).show();
        });
    }
    public static void requestText(String initial, int maximum) {
        LegionActivity activity = active;
        if (activity == null) { nativeSetInputResult(initial, false); return; }
        activity.runOnUiThread(() -> {
            EditText input = new EditText(activity);
            input.setSingleLine(true);
            input.setFilters(new InputFilter[] { new InputFilter.LengthFilter(maximum) });
            input.setText(initial);
            input.setSelection(input.length());
            AlertDialog dialog = new AlertDialog.Builder(activity).setTitle("Wpisz tekst").setView(input)
                .setPositiveButton("OK", (d, which) -> nativeSetInputResult(input.getText().toString(), true))
                .setNegativeButton("Anuluj", (d, which) -> nativeSetInputResult(initial, false))
                .setOnCancelListener(d -> nativeSetInputResult(initial, false)).create();
            dialog.setOnShowListener(d -> {
                input.requestFocus();
                dialog.getWindow().setSoftInputMode(android.view.WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
            });
            dialog.show();
        });
    }

    // The legacy SDL 1.2 entry point is exported as main, not SDL_main.
    @Override protected String getMainFunction() { return "main"; }
    @Override protected String[] getLibraries() {
        return new String[] { "SDL2", "SDL", "GL", "SDL_ttf", "SDL_image", "SDL_mixer", "main" };
    }
    @Override protected void onCreate(Bundle state) {
        try {
            File marker = new File(getFilesDir(), ".legion-assets-version");
            long version=getPackageManager().getPackageInfo(getPackageName(),0).lastUpdateTime;
            String installed="";
            if(marker.exists()) {
                try (FileInputStream in=new FileInputStream(marker)) {
                    byte[] bytes=new byte[32]; int count=in.read(bytes);
                    if(count>0) installed=new String(bytes,0,count,java.nio.charset.StandardCharsets.UTF_8);
                }
            }
            if(!installed.equals(Long.toString(version))) {
                copyAssets("runtime", getFilesDir());
                if(!new File(getFilesDir(), "legion/archiwum").isDirectory() &&
                   !new File(getFilesDir(), "legion/archiwum").mkdirs()) throw new IOException("Cannot create save directory");
                try (FileOutputStream out=new FileOutputStream(marker)) {
                    out.write(Long.toString(version).getBytes(java.nio.charset.StandardCharsets.UTF_8));
                }
            }
        } catch (IOException | android.content.pm.PackageManager.NameNotFoundException error) {
            Log.e("Legion", "Cannot prepare game data", error);
            super.onCreate(state);
            finish();
            return;
        }
        super.onCreate(state);
        active = this;
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN |
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        scaleDetector = new ScaleGestureDetector(this, new ScaleGestureDetector.SimpleOnScaleGestureListener() {
            @Override public boolean onScaleBegin(ScaleGestureDetector detector) {
                previousFocusX=surfaceX(detector.getFocusX())/touchWidth(); previousFocusY=surfaceY(detector.getFocusY())/touchHeight();
                return true;
            }
            @Override public boolean onScale(ScaleGestureDetector detector) {
                float fx=surfaceX(detector.getFocusX())/touchWidth(), fy=surfaceY(detector.getFocusY())/touchHeight();
                float worldX=(previousFocusX-.5f-panX)/zoom;
                float worldY=(previousFocusY-.5f-panY)/zoom;
                zoom=Math.max(1f,Math.min(4f,zoom*detector.getScaleFactor()));
                panX=fx-.5f-worldX*zoom; panY=fy-.5f-worldY*zoom;
                previousFocusX=fx; previousFocusY=fy; applyZoom(); return true;
            }
        });
    }
    @Override protected void onDestroy() { if (active==this) active=null; super.onDestroy(); }
    @Override public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus && active==this) nativeRequestRefresh();
    }
    @Override public boolean dispatchTouchEvent(MotionEvent event) {
        if (scaleDetector==null || event.getPointerCount()==0) return super.dispatchTouchEvent(event);
        int action=event.getActionMasked();
        boolean targeting=nativeTargetSelectionActive();
        if (action==MotionEvent.ACTION_DOWN) {
            int[] panelPosition=new int[2];
            if (moveControls!=null) moveControls.getLocationInWindow(panelPosition);
            routingMoveControls=targeting && moveControls!=null &&
                event.getY()>=panelPosition[1];
        }
        if (routingMoveControls) {
            boolean handled=super.dispatchTouchEvent(event);
            if (action==MotionEvent.ACTION_UP || action==MotionEvent.ACTION_CANCEL) routingMoveControls=false;
            return handled;
        }
        if (action==MotionEvent.ACTION_DOWN) {
            multiTouch=false; moved=false; rightPressed=false;
            lastX=downX=surfaceX(event.getX()); lastY=downY=surfaceY(event.getY());
            // Position the game pointer immediately; some original dialogs
            // resolve their hit target on the frame before the click.
            nativePointer(2,downX/touchWidth(),downY/touchHeight(),1);
            // The pointer can still be over the HUD when this first touch
            // returns it to the scene; record every new tactical gesture.
            if (!targeting && nativeTacticalSceneActive())
                nativeTacticalTouchBegin(downX/touchWidth(),downY/touchHeight());
            if (!targeting) touchHandler.postDelayed(longPress,500);
        } else if (action==MotionEvent.ACTION_POINTER_DOWN) {
            multiTouch=true; touchHandler.removeCallbacks(longPress);
            if (rightPressed) { nativePointer(1,downX/touchWidth(),downY/touchHeight(),2); rightPressed=false; }
        }
        if (!nativeTacticalSceneActive()) scaleDetector.onTouchEvent(event);
        if (action==MotionEvent.ACTION_MOVE && event.getPointerCount()==1 && !multiTouch) {
            float x=surfaceX(event.getX()),y=surfaceY(event.getY());
            float dx=x-downX,dy=y-downY;
            if (dx*dx+dy*dy>100) { moved=true; touchHandler.removeCallbacks(longPress); }
            if (targeting) {
                nativePointer(2,x/touchWidth(),y/touchHeight(),1);
                lastX=x; lastY=y;
                return true;
            }
            if (moved && !rightPressed) {
                float dxStep=(x-lastX)/touchWidth(),dyStep=(y-lastY)/touchHeight();
                if(nativeWorldMapActive()) {
                    float oldX=panX,oldY=panY;
                    if(zoom>1f) { panX+=dxStep; panY+=dyStep; applyZoom(); }
                    nativeWorldMapDrag(dxStep-(panX-oldX),dyStep-(panY-oldY));
                } else if(zoom>1f) { panX+=dxStep; panY+=dyStep; applyZoom(); }
            }
            // The original tactical UI switches between the toolbar and the
            // scene using pointer position, even when a drag has no click.
            nativePointer(2,x/touchWidth(),y/touchHeight(),rightPressed ? 2 : 1);
            lastX=x; lastY=y;
        }
        if (action==MotionEvent.ACTION_UP || action==MotionEvent.ACTION_CANCEL) {
            touchHandler.removeCallbacks(longPress);
            float x=surfaceX(event.getX())/touchWidth(),y=surfaceY(event.getY())/touchHeight();
            if (rightPressed) nativePointer(1,x,y,2);
            else if (action==MotionEvent.ACTION_UP && !multiTouch && (!moved || targeting)) {
                // Give the game a frame to update its hovered zone before
                // delivering the press, even for very quick taps.
                touchHandler.postDelayed(() -> nativePointer(0,x,y,1),35);
                touchHandler.postDelayed(() -> nativePointer(1,x,y,1),115);
            }
            rightPressed=false;
        }
        return true;
    }
    private void copyAssets(String source, File target) throws IOException {
        String[] children = getAssets().list(source);
        if (children != null && children.length > 0) {
            if (!target.isDirectory() && !target.mkdirs()) throw new IOException("Cannot create " + target);
            for (String child : children) copyAssets(source + "/" + child, new File(target, child));
        } else {
            try (InputStream in = getAssets().open(source); FileOutputStream out = new FileOutputStream(target)) {
                byte[] buffer = new byte[16384];
                int count;
                while ((count = in.read(buffer)) != -1) out.write(buffer, 0, count);
            }
        }
    }
    @Override protected String[] getArguments() {
        nativeSetWorkingDirectory(getFilesDir().getAbsolutePath());
        if ((getApplicationInfo().flags & android.content.pm.ApplicationInfo.FLAG_DEBUGGABLE)!=0) {
            if (getIntent().getBooleanExtra("legion_test_stats",false))
                nativeEnableTestStats();
            else if (getIntent().getBooleanExtra("legion_test_shop",false))
                nativeEnableTestShop(getIntent().getBooleanExtra("legion_test_granary",false));
            else if (getIntent().getBooleanExtra("legion_test_city",false))
                nativeEnableTestCity(getIntent().getBooleanExtra("legion_test_dialog",false));
            else if (getIntent().getBooleanExtra("legion_test_world",false)) nativeEnableTestWorld();
        }
        return new String[0];
    }
}
