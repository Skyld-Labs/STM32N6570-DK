STM32_PATH=/opt/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.300.202508131133/tools/bin/
PROG_PATH=/opt/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.300.202508131133/tools/bin/STM32_Programmer_CLI
DKEL=/opt/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.300.202508131133/tools/bin/ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr
SERVER_BIN="/opt/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.stlink-gdb-server.linux64_2.2.300.202509021040/tools/bin/ST-LINK_gdbserver"
SIGN_PATH="/opt/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.linux64_2.2.300.202508131133/tools/bin/STM32_SigningTool_CLI"

# --- 2. Build Phase ---
#echo "--- Building Project ---"
rm -rf /tmp/stm32_workspace_fsbl_cryp_saes
cd ./Projects/STM32N6570-DK/Examples/CRYP/CRYP_SAES_WrapKey || exit
/opt/st/stm32cubeide_2.0.0/stm32cubeide --launcher.suppressErrors -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data /tmp/stm32_workspace_fsbl_cryp_saes -import ./STM32CubeIDE/FSBL/ -build all
if [ $? -ne 0 ]; then echo "Build failed"; exit 1; fi

## --- 3. Signing Phase ---
echo "--- Signing Binary ---"
rm -f CRYP_SAES_WrapKey_FSBL-trusted.bin
$SIGN_PATH -bin STM32CubeIDE/FSBL/Debug/CRYP_SAES_WrapKey_FSBL.bin \
            -nk -of 0x80000000 -t fsbl \
            -o CRYP_SAES_WrapKey_FSBL-trusted.bin -hv 2.3 -dump CRYP_SAES_WrapKey_FSBL-trusted.bin

if [ $? -ne 0 ]; then echo "Signing failed"; exit 1; fi

## # --- 4. Final Flashing Phase (Signed Binary) ---
echo "--- Flashing Signed Project Binary ---"
$PROG_PATH -c port=SWD mode=HOTPLUG -el "$DKEL" -hardRst \
            -w CRYP_SAES_WrapKey_FSBL-trusted.bin 0x70100000 \
	    -w  "/home/adrianl/erase_key.bin" 0x77FFF000

if [ $? -ne 0 ]; then echo "Final flash failed"; exit 1; fi

$SERVER_BIN -p 61234 -l 1 -d -s -cp $STM32_PATH -m 1 -g

#/opt/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.linux64_1.0.100.202509120712/tools/bin/arm-none-eabi-gdb ./Projects/STM32N6570-DK/Examples/CRYP/CRYP_SAES_WrapKey/STM32CubeIDE/FSBL/Debug/CRYP_SAES_WrapKey_FSBL.elf
