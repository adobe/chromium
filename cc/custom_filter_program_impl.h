// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_CUSTOM_FILTER_PROGRAM_IMPL_H_
#define CC_CUSTOM_FILTER_PROGRAM_IMPL_H_

#include "base/memory/ref_counted.h"
#include "third_party/WebKit/Source/Platform/chromium/public/WebCustomFilterProgram.h"

namespace cc {

    class CustomFilterProgramImpl : 
        public base::RefCounted<CustomFilterProgramImpl>,
        public WebKit::WebCustomFilterProgram {
    public:
        static scoped_refptr<CustomFilterProgramImpl> create(const WebKit::WebCustomFilterProgram* program) {
            return make_scoped_refptr(new CustomFilterProgramImpl(program));
        }

        virtual WebKit::WebString vertexShader() const;
        virtual void setVertexShader(const WebKit::WebString&);

        virtual WebKit::WebString fragmentShader() const;
        virtual void setFragmentShader(const WebKit::WebString&);

    private:
        friend class base::RefCounted<CustomFilterProgramImpl>;
        CustomFilterProgramImpl(const WebKit::WebCustomFilterProgram*);
        virtual ~CustomFilterProgramImpl();

        virtual void refFromWebCustomFilterProgram();
        virtual void derefFromCustomFilterProgram();

        WebKit::WebString m_vertexShader;
        WebKit::WebString m_fragmentShader;
    };

}

#endif // CC_CUSTOM_FILTER_PROGRAM_IMPL_H_

