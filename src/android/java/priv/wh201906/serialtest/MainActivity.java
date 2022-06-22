package priv.wh201906.serialtest;

import org.qtproject.qt5.android.bindings.QtActivity;

import android.util.Log;
import android.view.WindowManager;
import android.content.Intent;
import android.os.Bundle;
import android.net.Uri;
import android.bluetooth.*;

import java.util.Set;
import java.lang.String;
import java.util.ArrayList;

public class MainActivity extends QtActivity
{
    private Intent startIntent = null;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        // keep screen on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        // store intent
        startIntent = getIntent();
        Log.i("onCreate", "");
        // handleIntent() should not be called there.
    }

    @Override
    protected void onNewIntent(Intent intent)
    {
        super.onNewIntent(intent);
        Log.i("onNewIntent", "");
        handleIntent(intent);
    }

    public void handleStartIntent()
    {
        if (startIntent != null)
        {
            handleIntent(startIntent);
            startIntent = null;
        }
    }

    private void handleIntent(Intent intent)
    {
        // debug output
        String action = intent.getAction();
        String type = intent.getType();
        Set<String> categories = intent.getCategories();
        Log.i("Action", action == null ? "(null)" : action);
        Log.i("Type", type == null ? "(null)" : type);
        if (categories == null)
            Log.i("Categorie", "(null)");
        else
        {
            for (String str : categories)
            {
                Log.i("Categorie", str);
            }
        }
        // check action
        if (action == null || !action.equals(Intent.ACTION_SEND) || type == null)
            return;
        // handle text
        if (type.startsWith("text/"))
        {
            Log.i("", "handle text");
            String text = intent.getStringExtra(Intent.EXTRA_TEXT);
            if (text == null)
                return;
            Log.i("text", text);
            shareText(text);
        }
        // handle file
        if (type.startsWith("application/") || type.startsWith("image/") || type.startsWith("video/") || type.startsWith("audio/"))
        {
            Log.i("", "handle file");
            Uri fileUri = (Uri) intent.getParcelableExtra(Intent.EXTRA_STREAM);
            if (fileUri == null)
                return;
            Log.i("fileUri", fileUri.toString());
        }
    }

    public String[] getBondedDevices(boolean isBLE)
    {
        BluetoothAdapter BTAdapter = BluetoothAdapter.getDefaultAdapter();
        Set<BluetoothDevice> bondedDevice = BTAdapter.getBondedDevices();
        ArrayList<String> list = new ArrayList<String>();
        for (BluetoothDevice bt : bondedDevice)
        {
            int deviceType = bt.getType();
            if (!isBLE && (deviceType == BluetoothDevice.DEVICE_TYPE_CLASSIC || deviceType == BluetoothDevice.DEVICE_TYPE_DUAL))
                list.add(bt.getAddress() + " " + bt.getName());
            else if (isBLE && (deviceType == BluetoothDevice.DEVICE_TYPE_LE || deviceType == BluetoothDevice.DEVICE_TYPE_DUAL))
                list.add(bt.getAddress() + " " + bt.getName());
        }
        String[] result = (String[]) list.toArray(new String[list.size()]);
        return result;
    }

    private static native void shareText(String text);
}
