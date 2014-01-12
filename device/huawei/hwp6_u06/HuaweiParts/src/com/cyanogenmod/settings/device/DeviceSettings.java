/*
 * Copyright (C) 2012 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.cyanogenmod.settings.device;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.PowerManager;
import android.os.SystemProperties;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.util.Log;


public class DeviceSettings extends PreferenceActivity implements
        OnPreferenceChangeListener {

    private CheckBoxPreference mSwitchStoragePref=null;

    private static final String TAG = "HuaweiParts";
    private static final String KEY_SWITCH_STORAGE = "key_switch_storage";
    private static final String PREFERENCE_CPU_MODE = "cpu_settings";
    private static final String CPU_PROPERTY = "sys.cpu.mode";

    private ListPreference mCpuMode;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.main_preferences);

        String mCurrCpuMode = "1";

        if (SystemProperties.get(CPU_PROPERTY) != null)
            mCurrCpuMode = SystemProperties.get(CPU_PROPERTY);

        mCpuMode = (ListPreference) getPreferenceScreen().findPreference(
                PREFERENCE_CPU_MODE);

        mCpuMode.setValueIndex(getCpuModeOffset(mCurrCpuMode));
        mCpuMode.setOnPreferenceChangeListener(this);

        mSwitchStoragePref = (CheckBoxPreference) getPreferenceScreen().findPreference(
            KEY_SWITCH_STORAGE);
        mSwitchStoragePref.setChecked((SystemProperties.getInt(
                "persist.sys.vold.switchexternal", 0) == 1));
        mSwitchStoragePref.setOnPreferenceChangeListener(this);

        if (SystemProperties.get("ro.vold.switchablepair","").equals("")) {
            mSwitchStoragePref.setSummaryOff(R.string.storage_switch_unavailable);
            mSwitchStoragePref.setEnabled(false);
        }
    }

    private int getCpuModeOffset(String mode) {
        if (mode.equals("0")) {
            return 0;
        } else if (mode.equals("2")) {
            return 2;
        } else {
            return 1;
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {

        if (preference == mCpuMode) {
            final String newCpuMode = (String) newValue;
            SystemProperties.set(CPU_PROPERTY, newCpuMode);
        }

        if(preference == mSwitchStoragePref) {
            Log.d(TAG,"Setting persist.sys.vold.switchexternal to "+(
                    mSwitchStoragePref.isChecked() ? "1" : "0"));
            SystemProperties.set("persist.sys.vold.switchexternal", (
                    (Boolean) newValue) ? "1" : "0");
            showRebootPrompt();
        }

        return true;
    }

    private void showRebootPrompt() {
        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle(R.string.reboot_prompt_title)
                .setMessage(R.string.reboot_prompt_message)
                .setPositiveButton(R.string.yes, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        PowerManager pm = (PowerManager) getSystemService(POWER_SERVICE);
                        pm.reboot(null);
                    }
                })
                .setNegativeButton(R.string.no, null)
                .create();

        dialog.show();
    }
}
