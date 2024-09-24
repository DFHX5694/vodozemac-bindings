use proc_macro::TokenStream;
use syn::{FnArg, Pat, PatType};
use quote::{quote, format_ident};
use quote::ToTokens;

#[proc_macro_attribute]
pub fn gen_noexcept(_attr: TokenStream, item: TokenStream) -> TokenStream {
    let function: syn::ItemFn = syn::parse(item.clone()).unwrap();

    // This generates the following code from a function definition:
    // fn FUNC_NAME_noexcept(ARGS) -> Box<Maybe<RETURN_TYPE>> {
    //     Box::new((FUNC_CALL).into())
    // }
    // where FUNC_CALL is:
    //   self.FUNC_NAME(ARG_NAMES), if this is a member function
    //   FUNC_NAME(ARG_NAMES), if this is a free function
    let syn::ReturnType::Type(_, return_type) = function.sig.output
        else { panic!("Not returning anything") };
    let input_types = function.sig.inputs.to_token_stream();
    let mut has_self = false;
    let mut args = syn::punctuated::Punctuated::<syn::Expr, syn::Token![,]>::new();

    // This loop is taken from https://github.com/dtolnay/no-panic/blob/master/src/lib.rs ,
    // originally licensed under Apache License 2.0
    // ( https://github.com/dtolnay/no-panic/blob/master/LICENSE-APACHE )
    // and MIT ( https://github.com/dtolnay/no-panic/blob/master/LICENSE-MIT )
    for arg in function.sig.inputs.iter() {
        match arg {
            FnArg::Typed(PatType { pat, .. })
            if match pat.as_ref() {
                Pat::Ident(pat) => pat.ident != "self",
                _ => true,
            } =>
            {
                let Pat::Ident(pat) = pat.as_ref() else { panic!("not pat ident") };
                let ident = pat.ident.clone();
                args.push(syn::parse2(quote!(#ident)).unwrap());
            }

            FnArg::Typed(_) | FnArg::Receiver(_) => {
                has_self = true;
            }
        }
    }

    let value_type = get_value_type(*(return_type.clone()));
    let func_name = function.sig.ident;
    let noexcept_func_name = format_ident!("{}_noexcept", func_name);
    let type_name: syn::Type = syn::parse2(quote!(Maybe<#value_type>)).unwrap();

    let function: proc_macro::TokenStream = if has_self {
        quote!{
            pub fn #noexcept_func_name(#input_types) -> Box<#type_name> {
                Box::new((self.#func_name(#args)).into())
            }
        }.into()
    } else {
        quote!{
            pub fn #noexcept_func_name(#input_types) -> Box<#type_name> {
                Box::new((#func_name(#args)).into())
            }
        }.into()
    };

    let res = TokenStream::from_iter([item, function]);
    res
}

// Input Result<T, E>, gets T
fn get_value_type(t: syn::Type) -> syn::Type {
    let syn::Type::Path(path) = t.clone() else { panic!("not a path") };
    let syn::PathArguments::AngleBracketed(ref args) = path.path.segments.first().expect("have first segment").arguments else { panic!("not angled") };
    let syn::GenericArgument::Type(value_type) = args.args.first().expect("have first arg") else { panic!("arg is not type") };
    value_type.clone()
}
