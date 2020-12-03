/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not use this file except in compliance
 * with the License. A copy of the License is located at
 *
 * http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions
 * and limitations under the License.
 */
package ai.djl.dlr.engine;

import ai.djl.Device;
import ai.djl.dlr.jni.JniUtils;
import ai.djl.inference.Predictor;
import ai.djl.translate.Translator;

public class DlrPredictor<I, O> extends Predictor<I, O> {

    public DlrPredictor(
            long dlrRuntimeId,
            DlrModel model,
            String modelDir,
            Device device,
            Translator<I, O> translator) {
        this.model = model;
        this.translator = translator;
        manager = model.getNDManager().newSubManager();
        manager.setName("predictor");
        long modelHandle = JniUtils.createDlrModel(dlrRuntimeId, modelDir, device);
        block = new DlrSymbolBlock(modelHandle);
    }
}
